#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

#ifndef __GLOBAL_CONSTANTS__
#include "../shared/constants.h"
#endif
#ifndef __GLOBAL_MESSAGING__
#include "../shared/messaging.c"
#endif
#ifndef __GLOBAL_FUNCTIONS__
#include "../shared/utility.c"
#endif
#ifndef __SERVER_CONSTANTS__
#include "./constants.h"
#endif
#ifndef __SERVER_FUNCTIONS__
#include "./utility.c"
#endif

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

  setup_epoll(&epoll_fd, (int[]){ this->pipe_fd[PR], this->user->socket_fd });

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

        if (new_message.size == 0) {
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

          announce.size = strlen(body);
          announce.body = malloc(announce.size);
          strcpy(announce.body, body);

          write(master_pipe[PW], &announce, sizeof(Message));
          goto exit_thread;

        } else if (new_message.type == TRANSFER_END) {
          // >> User had an error
          printf("User \"%s\" had transfer error.\n", this->user->username);

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
        free(message.body);
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