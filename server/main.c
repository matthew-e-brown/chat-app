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

// -- Local Includes

#include "../shared/constants.h"
#include "../shared/messaging.c"
#include "../shared/utility.c"

#include "./constants.h"
#include "./utility.c"
#include "./thread.c"
#include "./commands.c"

// -- Global Variables

Thread threads[CONN_LIMIT];     // Holds all client thread metadata
User users[CONN_LIMIT];         // Holds all users' metadata

pthread_mutex_t threads_lock;   // Locks access to 'threads'
pthread_mutex_t users_lock;     // Locks access to 'users'

int master_sock;                // Bound listener socket
struct sockaddr_in master_addr; // Address of the bound socket
socklen_t master_addr_size = sizeof(master_addr);  // Used for bind, size of ↑↑

int master_pipe[2];  // Pipe that threads can write to to message main thread

// -- Function definitions for this file

static void setup_listen_socket(int* socket_fd);
static int spawn_thread();

static void whisper(Message message);
static void broadcast(Message message);
static void run_command(Message message);

/**
 * Server's main funciton.
 * @return A status code; 0 on success
 */
int main() {
  int i, n;  // counter for loops
  int rc;    // re-useable return-code variable

  int epoll_fd;                                 // File descriptor for epoll
  int num_events;                               // Returned number of events
  struct epoll_event events[MAX_EPOLL_EVENTS];  // Returned events by epoll

  // >> Initialize main lists to NULL/"not in use" values
  memset(threads, 0, sizeof(Thread) * CONN_LIMIT);
  memset(users, 0, sizeof(User) * CONN_LIMIT);

  for (i = 0; i < CONN_LIMIT; i++) {
    users[i].socket_fd = -1;
    threads[i].in_use = 0;
  }

  // >> Initialize mutexes
  pthread_mutex_init(&threads_lock, NULL);
  pthread_mutex_init(&users_lock, NULL);

  // >> Create main pipe
  rc = pipe(master_pipe);

  if (rc) {
    perror("pipe");
    exit(1);
  }

  // >> Run socket setup steps
  setup_listen_socket(&master_sock);

  printf("%s Server listening on port %i (socket FD of %i)\n",
    timestamp(), PORT, master_sock);

  // >> Run epoll setup steps
  rc = setup_epoll(&epoll_fd, (int[]){ master_sock, master_pipe[PR] });

  if (rc != 0) {
    switch (rc) {
      case -1: perror("epoll_create1"); break;
      case 1: perror("epoll_add master_sock"); break;
      case 2: perror("epoll_add master_pipe"); break;
    }
    exit(1);
  }

  while (1) {
    // >> Wait for epoll events

    num_events = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);

    if (num_events == -1) {
      perror("epoll wait");
      close(master_sock);
      exit(1);
    }

    for (n = 0; n < num_events; n++) {

      if (events[n].data.fd == master_sock) {
        // >> There is a new connection
        spawn_thread();

      } else if (events[n].data.fd == master_pipe[PR]) {

        // >> There is a thread ping
        Message from_thread;
        read(master_pipe[PR], &from_thread, sizeof(Message));

        // >> Redirect message accordingly
        switch (from_thread.type) {
          case SRV_ANNOUNCE:   // A thread is trying to announce something
          case MSG_BROADCAST:  // A user is attempting to broadcast to others
            broadcast(from_thread);
            break;

          case MSG_WHISPER:    // A user is whispering
            whisper(from_thread);
            break;

          case MSG_COMMAND:    // A user is running a command
            run_command(from_thread);
            break;

          default:             // Unknown/inappropriate message type
            fprintf(stderr, "Received incorrect message type.\n");

            Message response;
            const char error[] = "Invalid message type.";
            Thread* culprit = get_thread_by_username(from_thread.sender_name);

            if (culprit == NULL) {
              fprintf(stderr, "Its sending thread couldn't be found.\n");
              break;
            }

            response.type = USR_ERROR;
            response.size = strlen(error);
            response.body = malloc(response.size);
            strcpy(response.body, error);

            memset(response.sender_name, 0, USERNAME_MAX);
            memset(response.receiver_name, 0, USERNAME_MAX);

            // >> Send back down to thread
            write(culprit->pipe_fd[PW], &response, sizeof(Message));

            break;
        }

      }

    } // end of for-events

  } // end of main while-loop

  return 0;
}


/**
 * Whispers a message based on the receiving name.
 * @param message The message to send
 */
static void whisper(Message message) {
  Thread* destination = get_thread_by_username(message.receiver_name);

  if (destination != NULL) {
    write(destination->pipe_fd[PW], &message, sizeof(Message));
  } else {
    // >> Uh oh! Couldn't find user
    Message response;
    const char error[] = "Could not find a user with that name.";
    Thread* culprit = get_thread_by_username(message.sender_name);

    response.type = USR_ERROR;
    response.size = strlen(error);
    response.body = malloc(response.size);
    strcpy(response.body, error);

    memset(response.sender_name, 0, USERNAME_MAX);
    memset(response.receiver_name, 0, USERNAME_MAX);

    send_message(culprit->user->socket_fd, response);
    free(response.body);
  }
}


/**
 * Sends the given message to all clients.
 * @param message The message to send
 */
static void broadcast(Message message) {
  int i;

  // >> Get source so as to not re-send to source user
  Thread* source = get_thread_by_username(message.sender_name);

  pthread_mutex_lock(&users_lock);
  pthread_mutex_lock(&threads_lock);

  // >> Send all the messages to their threads
  for (i = 0; i < CONN_LIMIT; i++) {
    if (threads[i].in_use && threads + i != source) {
      write(threads[i].pipe_fd[PW], &message, sizeof(Message));
    }
  }

  pthread_mutex_unlock(&threads_lock);
  pthread_mutex_unlock(&users_lock);
}


/**
 * Runs a command from the client and sends the response back.
 * @param message The message of which the body contains a command string.
 */
static void run_command(Message message) {
  Message response;

  // >> Find command
  command_ptr command = find_command(message.body);
  if (command == NULL) {
    response.type = USR_ERROR;
    goto error;
  }

  // >> Run command and get return code
  int rc = (*command)(&response);
  if (rc) {
    response.type = SRV_ERROR;
    goto error;
  }

  goto send_response; // Skip over creating error packet if error not hit

error:;
  response.size = 0;
  response.body = NULL;
  memset(response.sender_name, 0, USERNAME_MAX);
  memset(response.receiver_name, 0, USERNAME_MAX);

send_response:;
  Thread* reply_to = get_thread_by_username(message.sender_name);
  write(reply_to->pipe_fd[PW], &response, sizeof(Message));

  // Hmmm... maybe I am getting too comfortable with goto statements. Oh well, I
  // like them. Maybe I should write more Assembly, lol.
}


/**
 * Does socket setup. Extracted here to keep main() tidy.
 */
static void setup_listen_socket(int* socket_fd) {
  int rc;

  // >> Create master socket
  *socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (*socket_fd < 0) {
    perror("socket creation");
    exit(1);
  }

  // >> Set properties for socket address
  master_addr.sin_family = AF_INET;
  master_addr.sin_port = htons(PORT);
  master_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // >> Bind the socket
  rc = bind(*socket_fd, (struct sockaddr*)&master_addr, master_addr_size);

  if (rc) {
    perror("socket bind");
    exit(1);
  }

  // >> Start listening on the socket
  rc = listen(*socket_fd, CONN_LIMIT);

  if (rc) {
    perror("socket listen");
    exit(1);
  }
}


/**
 * Logs the user in and creates a thread for them.
 * @return A return code; 0 on success, 1 otherwise
 */
static int spawn_thread() {
  int i, rc;
  char res_msg[48];
  Message request, response;

  int client_sock = accept(
    master_sock, (struct sockaddr*)&master_addr, &master_addr_size
  );

  if (client_sock == -1) {
    perror("accept new client");
    return -1;
  }

  // >> Receive login request containing username
  request = recv_message(client_sock);

  printf("%s Received login request\n", timestamp());

  // Free memory if it was set, just in case
  if (request.body != NULL) free(request.body);

  // Check that they are actually logging in
  if (request.type != MSG_LOGIN) {
    response.type = USR_ERROR;
    strcpy(res_msg, "Need to login before anything else");
    goto send_response;
  }

  // >> Find the first empty spot in the Users array to put this new connection

  pthread_mutex_lock(&users_lock);
  pthread_mutex_lock(&threads_lock);

  for (i = 0; i <= CONN_LIMIT; i++) {
    if (i == CONN_LIMIT) { // Didn't find any free spots
      fprintf(stderr, "Max connections reached, rejecting connection\n");

      pthread_mutex_unlock(&threads_lock);
      pthread_mutex_unlock(&users_lock);

      response.type = SRV_ERROR;
      strcpy(res_msg, "Server is full");
      goto send_response;
    } else if (users[i].socket_fd == -1) break; // i is a free spot
  }

  // >> Store user information and release users array
  users[i].socket_fd = client_sock;
  strncpy(users[i].username, request.sender_name, USERNAME_MAX);
  User* new_user = users + i; // keep track of user pointer

  // >> Find the first empty spot in the Threads array to put this new user in

  for (i = 0; i <= CONN_LIMIT; i++) {
    if (i == CONN_LIMIT) {
      fprintf(stderr, "Max threads reached, rejecting connection\n");

      users[i].socket_fd = -1;
      memset(users[i].username, 0, USERNAME_MAX);

      pthread_mutex_unlock(&threads_lock);
      pthread_mutex_unlock(&users_lock);

      response.type = SRV_ERROR;
      strcpy(res_msg, "Server is full");
      goto send_response;
    } else if (!threads[i].in_use) break; // i is a free spot
  }

  // >> Initialize thread

  rc = pipe(threads[i].pipe_fd);
  if (rc) {
    perror("thread pipe creation");

    response.type = SRV_ERROR;
    strcpy(res_msg, "Something went wrong");
    goto send_response;
  }

  threads[i].in_use = 1;
  threads[i].user = new_user;

  // >> Create thread and release threads array
  pthread_create(&threads[i].id, NULL, client_thread, (void*)&threads[i]);

  pthread_mutex_unlock(&threads_lock);
  pthread_mutex_unlock(&users_lock);

  // >> Respond to client
  response.type = SRV_RESPONSE;
  memset(res_msg, 0, sizeof(res_msg));

  // Jump here to send response to client
send_response:
  response.size = strlen(res_msg);

  if (response.size > 0) {
    response.body = malloc(response.size);
    strcpy(response.body, res_msg);
  }

  memset(response.sender_name, 0, USERNAME_MAX);
  memset(response.receiver_name, 0, USERNAME_MAX);

  send_message(client_sock, response);
  if (response.size > 0) free(response.body);

  if (response.type != SRV_RESPONSE) {
    close(client_sock);
    return 1;
  } else {
    return 0;
  }
}