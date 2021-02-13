/**
 * COIS-4310H: Chat App
 *
 * @name:         client_thread.c
 *
 * @author:       Matthew Brown; #0648289
 * @date:         February 1st to February 12th, 2021
 *
 * @purpose:      This file contains basically one function. This function is
 *                the one that is passed to each pthread, and it is responsible
 *                for listening to and responding to exactly one connected
 *                client socket.
 */

#define CLIENT_EPOLL_MAX 10

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

#ifndef __GLOBAL__
#include "../global.c"
#endif
#ifndef __SERVER_CONSTANTS__
#include "./constants.h"
#endif
#ifndef __USERS__
#include "./users.c"
#endif

// -- Global variables from main.c

extern int master_pipe[2];

extern User connected_users[MAX_CONNECTED_USERS];    // all users
extern pthread_mutex_t users_lock;                   // lock for users
extern Thread client_threads[MAX_CONNECTED_USERS];   // all threads
extern pthread_mutex_t thread_lock;                  // lock for threads


void* client_thread(void* arg) {
  Thread* this = (Thread*)arg;

  // printf("thread pointer: %p\n", this);
  // printf("user pointer: %p\n", this->user);

  // printf("start of thread\n");

  int return_code;
  struct epoll_event ev, events[CLIENT_EPOLL_MAX];  // epoll events
  int num_events, n;                // epoll return event count and index
  int epoll_fd = epoll_create1(0);  // FD of epoll itself

  // >> add both FDs to epoll

  ev.events = EPOLLIN;
  ev.data.fd = this->pipe_fd[PR];

  return_code = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->pipe_fd[PR], &ev);

  if (return_code) {
    perror("thread epoll_ctl add this->pipe_fd");
    goto exit_thread;
  }

  ev.events = EPOLLIN;
  ev.data.fd = this->user->socket_fd;

  return_code = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, this->user->socket_fd, &ev);

  if (return_code) {
    perror("thread epoll_ctrl add this->user->socket");
    goto exit_thread;
  }

  while (1) {
    num_events = epoll_wait(epoll_fd, events, CLIENT_EPOLL_MAX, -1);

    if (num_events == -1) {
      perror("epoll_wait");
      goto exit_thread;
    }

    // >> Check each event

    int bytes_read;
    Packet message;

    for (n = 0; n < num_events; n++) {
      if (events[n].data.fd == this->user->socket_fd) {
        // >> Message on the socket

        bytes_read = recv(this->user->socket_fd, &message, sizeof message, 0);

        // >> Either: (nothing received | error) --> client disconnected
        if (bytes_read <= 0) {

          // >> Get username to print before freeing any memory
          pthread_mutex_lock(&users_lock);

          User* user = get_user_by_sock(
            connected_users,
            MAX_CONNECTED_USERS,
            this->user->socket_fd
          );

          char username[MAX_NAME_LEN];
          strncpy(username, user->username, MAX_NAME_LEN);

          pthread_mutex_unlock(&users_lock);

          if (bytes_read < 0) {
            perror("recv");
            fprintf(stderr, "User \"%s\" errored: disconnecting.\n", username);
          } else {
            printf("User \"%s\" disconnecting.\n", username);
          }

          message.header = create_header(MSG_ANNOUNCE, 0, 0);
          message.header.packet_count = 1;
          message.header.packet_index = 0;
          snprintf(message.body, MAX_MSG_LEN, "%s has left.", username);

          write(master_pipe[PW], &message, sizeof message);

          goto exit_thread;
        }

        // Bytes were read! We have a message. Redirect it up to the master
        // thread!
        else {
          write(master_pipe[PW], &message, sizeof message);
        }

      } else if (events[n].data.fd == this->pipe_fd[PR]) {
        // >> message from master to send

        bytes_read = read(this->pipe_fd[PR], &message, sizeof message);
        if (bytes_read != sizeof message) {
          fprintf(stderr, "Reading from master returned incorrect bytes?\n");
          continue; // skip this message
        }

        // >> redirect the message from master thread to client
        send(this->user->socket_fd, &message, sizeof message, 0);

      }
    }
  }

  // All error conditions need to do the same thing
  exit_thread:
    // printf("Exiting thread\n");
    pthread_mutex_lock(&users_lock);
    pthread_mutex_lock(&thread_lock);

    this->in_use = 0;
    close(this->pipe_fd[PR]);
    close(this->pipe_fd[PW]);
    close(this->user->socket_fd);
    this->user->socket_fd = -1; // "false" value

    pthread_mutex_unlock(&users_lock);
    pthread_mutex_unlock(&thread_lock);
    pthread_exit(NULL);
}