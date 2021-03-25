/**
 * COIS-4310H: Chat App
 *
 * @name:         Chat App -- Server thread function
 *
 * @author:       Matthew Brown, #0648289
 * @date:         February 1st to February 12th, 2021  
 *                March 1st to March 9th, 2021
 *
 * @purpose:      The code defined in this file is the code run in each thread:
 *                one for each client connected. It listens for incoming
 *                messages from the client and redirects them up to the main
 *                thread. It also listens to the main thread and redirects
 *                messages from there to the client.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>
#include <sys/epoll.h>

#include "../shared/constants.h"
#include "../shared/messaging.h"
#include "../shared/utility.h"
#include "./constants.h"
#include "./utility.h"

extern int master_pipe[2];

extern User users[CONN_LIMIT];
extern Thread threads[CONN_LIMIT];
extern pthread_mutex_t users_lock;
extern pthread_mutex_t threads_lock;

/**
 * Handles ongoing communication between a client and the server
 * @param arg A void pointer to the thread struct this thread is responsible for
 */
void* client_thread(void* arg) {
  Thread* this = (Thread*)arg;

  int n, epoll_fd, num_events;
  struct epoll_event events[MAX_EPOLL_EVENTS];

  int rc = setup_epoll(
    &epoll_fd, (int[]){ this->pipe_fd[PR], this->user->socket_fd }, 2
  );

  // >> If failed to establish epoll listen
  if (rc) {
    Message to_client;
    Message rejection;
    char to_client_message[36];
    char rejection_message[32 + USERNAME_MAX];

    switch (rc) {
      case -1: perror("epoll_create1 in new client thread"); break;
      case 1: perror("epoll_add thread->pipe_fd"); break;
      case 2: perror("epoll_add thread->user->socket"); break;
    }

    printf("%s User \"%s\"'s thread could not be created.\n",
      timestamp(), this->user->username);

    // >> Inform new client and other clients that connection was not successful

    to_client.type = SRV_ERROR;
    rejection.type = SRV_ANNOUNCE;

    memset(to_client.sender_name, 0, USERNAME_MAX);
    memset(rejection.sender_name, 0, USERNAME_MAX);
    memset(to_client.receiver_name, 0, USERNAME_MAX);
    memset(rejection.receiver_name, 0, USERNAME_MAX);

    sprintf(to_client_message, "Server could not establish thread.");
    sprintf(rejection_message, "User \"%s\" could not be logged in.",
      this->user->username);

    to_client.size = strlen(to_client_message) + 1;
    rejection.size = strlen(rejection_message) + 1;

    to_client.body = calloc(to_client.size, 1);
    rejection.body = calloc(rejection.size, 1);

    // >> Send the messages
    send_message(this->user->socket_fd, to_client);
    write(master_pipe[PW], &rejection, sizeof(Message));

    goto exit_thread;
  }


  while (1) {
    num_events = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);

    if (num_events == -1) {
      char error[24];
      sprintf(error, "thread %i epoll_wait", (int)this->id);
      perror(error);
      goto exit_thread;
    }

    for (n = 0; n < num_events; n++) {
      if (events[n].data.fd == this->user->socket_fd) {
        // >> New message on socket
        Message new_message = recv_message(this->user->socket_fd);

        if (new_message.type == MSG_UNSET) {
          // >> User logged out

          printf("%s User \"%s\" disconnecting.\n",
            timestamp(), this->user->username);

          // >> Announce to other users that this user has disconnected
          Message announce;
          announce.type = SRV_ANNOUNCE;
          memset(announce.sender_name, 0, USERNAME_MAX);
          memset(announce.receiver_name, 0, USERNAME_MAX);

          // >> Format output
          char body[28 + USERNAME_MAX]; // "User ... has ..." = 26, 28 in case
          sprintf(body, "User \"%s\" has disconnected.", this->user->username);

          announce.size = strlen(body) + 1;
          announce.body = calloc(announce.size, 1);
          strcpy(announce.body, body);

          write(master_pipe[PW], &announce, sizeof(Message));
          goto exit_thread;

        } else if (new_message.type == TRANSFER_END) {
          // >> User had an error
          printf("%s User \"%s\" had transfer error.\n",
            timestamp(), this->user->username);

          // No need to boot them off. TRANSFER_END happens when *they* leave
          // due to an error, so no need to respond either; they already know.

        } else {
          // >> User sent a message properly, forward to main for routing
          write(master_pipe[PW], &new_message, sizeof(Message));
        }

      } else if (events[n].data.fd == this->pipe_fd[PR]) {
        // >> Message from main thread to send
        Message message;

        read(this->pipe_fd[PR], &message, sizeof(Message));
        send_message(this->user->socket_fd, message);

        if (message.body != NULL) free(message.body);
      }
    }

  }

exit_thread:
  pthread_mutex_lock(&users_lock);
  pthread_mutex_lock(&threads_lock);

  this->in_use = 0;
  close(this->pipe_fd[PR]);
  close(this->pipe_fd[PW]);
  close(this->user->socket_fd);
  this->user->socket_fd = -1;

  pthread_mutex_unlock(&threads_lock);
  pthread_mutex_unlock(&users_lock);

  pthread_exit(NULL);
}