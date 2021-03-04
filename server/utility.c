#define __SERVER_FUNCTIONS__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/epoll.h>

#include <pthread.h>

#ifndef __GLOBAL_CONSTANTS__
#include "../shared/constants.h"
#endif
#ifndef __SERVER_CONSTANTS__
#include "./types.h"
#endif

extern Thread threads[CONN_LIMIT];
extern User users[CONN_LIMIT];
extern pthread_mutex_t threads_lock;
extern pthread_mutex_t users_lock;

/**
 * Sets up a bunch of file listeners on a specific epoll fd.
 * @param epoll_fd A pointer to the epoll_fd to set
 * @param files All the file descriptors to listen to on epoll
 * @return 0 on success, -1 on epoll_create failure, and index of files array on
 * any other failure (1 indexed, to avoid collision w/ 0)
 */
int setup_epoll(int* epoll_fd, int files[]) {
  int i, rc;

  struct epoll_event event;

  *epoll_fd = epoll_create1(0);

  if (*epoll_fd == -1) return -1;

  for (i = 0; i < NUM_ELEMS(files); i++) {
    event.events = EPOLLIN;
    event.data.fd = files[i];

    rc = epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, files[i], &event);

    if (rc) return i + 1;
  }

  return 0;
}


Thread* get_thread_by_username(const char username[]) {
  int i;
  Thread* result;

  pthread_mutex_lock(&users_lock);
  pthread_mutex_lock(&threads_lock);

  for (i = 0; i <= CONN_LIMIT; i++) {
    if (strncmp(threads[i].user->username, username, USERNAME_MAX) == 0) {
      result = threads + i;
      break;
    } else if (i == CONN_LIMIT) {
      result = NULL;
      break;
    }
  }

  pthread_mutex_unlock(&threads_lock);
  pthread_mutex_unlock(&users_lock);

  return result;
}