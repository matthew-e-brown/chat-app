/**
 * COIS-4310H: Chat App
 *
 * @name:         Chat App -- Global utility functions
 *
 * @author:       Matthew Brown, #0648289
 * @date:         February 1st to February 12th, 2021  
 *                March 1st to March 9th, 2021
 *
 * @purpose:      This file holds functions that are used by both the client and
 *                the server.
 *
 */

#include <time.h>
#include <sys/types.h>
#include <sys/epoll.h>

#include "./constants.h"


int setup_epoll(int* epoll_fd, int files[], int count) {
  int i, rc;

  struct epoll_event event;

  *epoll_fd = epoll_create1(0);

  if (*epoll_fd == -1) return -1;

  for (i = 0; i < count; i++) {
    event.events = EPOLLIN;
    event.data.fd = files[i];

    rc = epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, files[i], &event);

    if (rc) return i + 1;
  }

  return 0;
}


char* timestamp() {
  static char timestamp[22];

  time_t rawtime = time(NULL);
  struct tm* timeinfo = localtime(&rawtime);

  strftime(timestamp, 22, "[%F %T]", timeinfo);
  return timestamp;
}