// -- Includes

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/epoll.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

// -- My includes

#include "../global.c"
#include "./constants.h"

#include "./users.c"
#include "./commands.c"
#include "./client_thread.c"

// -- Function headers for this file

User* log_in_new_user(int, struct sockaddr*, socklen_t*);

// -- Global variables

// Holds all the client threads
Thread client_threads[MAX_CONNECTED_USERS];
// Locks access to the client_threads array
pthread_mutex_t thread_lock;

// Holds every connected user
User connected_users[MAX_CONNECTED_USERS];
// Locks access to the connected_users array
pthread_mutex_t users_lock;

// Bound socket that listens for new connections
int master_sock;
// The address of the bound listener socket
struct sockaddr_in master_addr;
// The size of the master socket's address. Used for bind
socklen_t master_addr_size = sizeof(master_addr);
// Pipe to message from threads to master
int master_pipe[2];


/**
 * The server's main function.
 * @returns an int; a return code. Zero on no error.
 */
int main() {
  int i;
  int return_code;  // Return code from various functions

  // >> Initialize the main lists to NULL

  memset(connected_users, 0, sizeof(User) * MAX_CONNECTED_USERS);
  memset(client_threads, 0, sizeof(Thread) * MAX_CONNECTED_USERS);

  // >> Initialize thread and users arrays to correct non-running states
  for (i = 0; i < MAX_CONNECTED_USERS; i++) {
    // (these are the values that are checked against to see if a spot in the
    // array is "free")
    connected_users[i].socket_fd = -1;
    client_threads[i].in_use = 0;
  }

  // >> Create the master socket

  master_sock = socket(AF_INET, SOCK_STREAM, 0);

  if (master_sock < 0) {
    perror("socket create");
    exit(1);
  }

  // >> Set properties for the master socket's address

  master_addr.sin_family = AF_INET;
  master_addr.sin_port = htons(PORT);
  master_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // >> Bind the socket

  return_code = bind(
    master_sock, (struct sockaddr*)&master_addr, master_addr_size
  );

  if (return_code) {
    perror("socket bind");
    exit(1);
  }

  // >> Start listening on the socket

  return_code = listen(master_sock, MAX_CONNECTED_USERS);

  if (return_code) {
    perror("socket listen");
    exit(1);
  }

  printf("server with FD %i now listening on port %i\n", master_sock, PORT);

  // ================================================= SETUP PART ONE COMPLETE

  // >> Set up epoll listener for master_sock

  struct epoll_event ev, events[MAX_EPOLL_EVENTS];  // epoll events
  int num_events, n;                // epoll return event count and index
  int epoll_fd = epoll_create1(0);  // FD of epoll itself

  if (epoll_fd == -1) {
    perror("epoll create");
    exit(1);
  }

  ev.events = EPOLLIN;
  ev.data.fd = master_sock;

  return_code = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, master_sock, &ev);

  if (return_code) {
    perror("epoll add master_sock");
    exit(1);
  }

  // >> Set up master_pipe and add to epoll

  return_code = pipe(master_pipe);
  if (return_code) {
    perror("pipe");
    exit(1);
  }

  ev.events = EPOLLIN;
  ev.data.fd = master_pipe[PR];

  return_code = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, master_pipe[PR], &ev);

  if (return_code) {
    perror("epoll add master_pipe");
    exit(1);
  }

  // ================================================= SETUP PART TWO COMPLETE

  pthread_mutex_init(&thread_lock, NULL);
  pthread_mutex_init(&users_lock, NULL);

  while (1) {
    // >> Wait for events on epoll

    num_events = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);

    if (num_events == -1) {
      perror("epoll_wait");
      exit(1);
    }

    // >> Check the events on each FD

    for (n = 0; n < num_events; n++) {
      if (events[n].data.fd == master_sock) {
        // >> Bound socket has event -- new client

        User* new_user = log_in_new_user(
          master_sock, (struct sockaddr*)&master_addr, &master_addr_size
        );

        if (new_user == NULL) goto abandon_event;

        // loop through and set i to the free spot, then break
        for (i = 0; i <= MAX_CONNECTED_USERS; i++) {
          if (i == MAX_CONNECTED_USERS) {
            fprintf(stderr, "Attempting to make new thread when full!!\n");
          } else if (!client_threads[i].in_use) break; // free spot
        }

        // >> Make the new thread object

        pthread_mutex_lock(&thread_lock);
        client_threads[i].in_use = 1;
        client_threads[i].user = new_user;
        return_code = pipe(client_threads[i].pipe_fd);
        pthread_mutex_unlock(&thread_lock);

        if (return_code) {
          perror("new thread pipe");
        }

        // >> spawn the child thread
        pthread_create(
          &client_threads[i].thread_id,
          NULL,
          client_thread,
          (void*)&client_threads[i]
        );

        Packet message;
        message.header = create_header(MSG_ANNOUNCE, 0, 0);
        message.header.packet_count = 1;
        message.header.packet_count = 0;
        snprintf(
          message.body, MAX_MSG_LEN,
          "%s has logged in!", new_user->username
        );

        // >> Announce to the other threads
        pthread_mutex_lock(&thread_lock);

        for (i = 0; i < MAX_CONNECTED_USERS; i++) {
          if (client_threads[i].in_use && client_threads[i].user != new_user) {
            write(client_threads[i].pipe_fd[PW], &message, sizeof message);
          }
        }

        pthread_mutex_unlock(&thread_lock);

        printf("New user \"%s\" has logged in\n", new_user->username);

      } else if (events[n].data.fd == master_pipe[PR]) {
        // >> Thread-to-master pipe -- there's a message to redirect

        int bytes_read;
        Packet message;

        bytes_read = read(master_pipe[PR], &message, sizeof message);

        if (bytes_read != sizeof message) {
          if (bytes_read == -1)
            perror("read from pipe to master");
          else
            fprintf(stderr, "Reading from master_pipe gave incorrect bytes?\n");
          goto abandon_event;
        }

        // >> Find which thread sent that before we overwrite the message's
        // >> fields, in case we need it
        pthread_mutex_lock(&users_lock);
        pthread_mutex_lock(&thread_lock);

        Thread* original_sender = get_thread_by_user(
          client_threads, MAX_CONNECTED_USERS, get_user_by_name(
            connected_users, MAX_CONNECTED_USERS,
            message.header.sender_name
          )
        );

        pthread_mutex_unlock(&thread_lock);
        pthread_mutex_unlock(&users_lock);

        // >> determine who to send to

        if (
          message.header.message_type == MSG_BROADCAST ||
          message.header.message_type == MSG_ANNOUNCE
        ) {

          // >> send to all client threads
          for (i = 0; i < MAX_CONNECTED_USERS; i++) {

            // >> don't re-send to the sending client
            pthread_mutex_lock(&users_lock);
            User *sender = get_user_by_name(
              connected_users, MAX_CONNECTED_USERS, message.header.sender_name
            );
            pthread_mutex_unlock(&users_lock);

            pthread_mutex_lock(&thread_lock);
            if (client_threads[i].in_use && client_threads[i].user != sender) {
              write(client_threads[i].pipe_fd[PW], &message, sizeof message);
            }
            pthread_mutex_unlock(&thread_lock);
          }

        } // end of if broadast

        else if (message.header.message_type == MSG_COMMAND) {

          printf("Finding command\n");

          // >> determine which command they want
          command_pt command = get_command(message.body);

          int response; // default "from server" type

          if (command == NULL) {
            // command not found, print into buffer saying it couldn't be found
            response = MSG_ACK_WITH_ERR;
            strncpy(
              message.body + strnlen(message.body, MAX_COMMAND_LENGTH),
              " is not recognized as a command",
              MAX_MSG_LEN - MAX_COMMAND_LENGTH
            );
          } else if (  (*command)(&message)  ) {
            // command failed
            response = MSG_ACK_WITH_ERR;
            snprintf(message.body, MAX_MSG_LEN, "command failed on server");
          } else {
            // command succeeded
            response = MSG_ANNOUNCE;
          }

          message.header = create_header(
            response, 0, message.header.sender_name
          );
          message.header.packet_count = 1;
          message.header.packet_index = 0;

          write(original_sender->pipe_fd[PW], &message, sizeof message);

        } // end of if command

        else if (message.header.message_type == MSG_WHISPER) {
          // >> find the user to whisper to

          pthread_mutex_lock(&users_lock);
          User *dest = get_user_by_name(
            connected_users, MAX_CONNECTED_USERS, message.header.receiver_name
          );
          pthread_mutex_unlock(&users_lock);

          if (dest == NULL) {
            // >> there is no user with that username, send ACK_WITH_ERR back

            snprintf(
              message.body, MAX_MSG_LEN, "There is no user named \"%s\".",
              message.header.receiver_name
            );

            message.header = create_header(
              MSG_ACK_WITH_ERR, 0, message.header.sender_name
            );

            message.header.packet_count = 1;
            message.header.packet_index = 0;

            write(original_sender->pipe_fd[PW], &message, sizeof message);

          } else {
            // >> user was found
            pthread_mutex_lock(&thread_lock);
            for (i = 0; i <= MAX_CONNECTED_USERS; i++) {
              if (i == MAX_CONNECTED_USERS) {
                fprintf(
                  stderr, "User %p's thread not found. Should never happen!!\n",
                  dest
                );
                goto abandon_event;
              } else if (client_threads[i].user == dest) break; // i == the dest
            }

            write(client_threads[i].pipe_fd[PW], &message, sizeof message);
            pthread_mutex_unlock(&thread_lock);
          }

        } // end of if whisper

      }

      // jump here to abandon this event. Used instead of continue to be able to
      // jump out of nested loops
      abandon_event:;

    } // END OF for-each event
  } // END OF main while loop

  close(master_sock);

  return 0;
}


/**
 * Accepts a new connection, receives the first message, then sends an
 * acknowledgement. Places the new User in the connected_users array, then
 * returns its pointer
 * @param socket_fd the socket on which to receive the new connection
 * @param address the address of the socket
 * @param size the size of the address
 * @return NULL on error, pointer to the user in the array if success
 */
User* log_in_new_user(int socket_fd, struct sockaddr* address, socklen_t* size) {
  Packet message;
  int i, error = 0;
  int client_sock = accept(socket_fd, address, size);

  if (client_sock == -1) {
    perror("accept new client");
    return NULL;
  }

  for (i = 0; i <= MAX_CONNECTED_USERS; i++) {
    if (i == MAX_CONNECTED_USERS) {
      printf("Max connections reached, not accepting new users\n");
      return NULL;
    } else if (connected_users[i].socket_fd == -1) break;
    // break when 'i' is a free array space
  }

  // >> Get message from client to get their username

  // check errors
  if (!recv(client_sock, &message, sizeof message, 0)) {

    // Bytes weren't read for some reason
    fprintf(stderr, "User almost connected, but got no welcome message.\n");
    return NULL;

  }

  else if (message.header.application_version != APP_VER) error = 'V';
  else if (message.header.message_type != MSG_LOGIN) error = 'L';

  if (error) {
    // if there was an error
    message.header = create_header(MSG_ACK_WITH_ERR, 0, message.header.sender_name);
    message.header.packet_count = 1;
    message.header.packet_index = 0;

    switch (error) {
      case 'V':
        strncpy(
          message.body,
          "Server replied with error: Incorrect application version",
          MAX_MSG_LEN
        );
        break;
      case 'L':
        strncpy(
          message.body,
          "Server replied with error: Must login first",
          MAX_MSG_LEN
        );
        break;
    }
  } else {
    // No error

    // >> Save the user's information on the server side
    pthread_mutex_lock(&users_lock);
    connected_users[i].socket_fd = client_sock;
    strncpy(connected_users[i].username, message.header.sender_name, MAX_NAME_LEN);
    pthread_mutex_unlock(&users_lock);

    // >> Format a response
    message.header = create_header(MSG_ACKNOWLEDGE, 0, message.header.sender_name);
    message.header.packet_count = 1;
    message.header.packet_index = 0;

  }

  // >> Send acknlowedgement
  send(client_sock, &message, sizeof message, 0);
  return error ? NULL : &connected_users[i];
}