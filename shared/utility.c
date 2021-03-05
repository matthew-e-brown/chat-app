#define __GLOBAL_FUNCTIONS__

#include <time.h>
#include <sys/types.h>
#include <sys/epoll.h>

#ifndef __GLOBAL_CONSTANTS__
#include "./constants.h"
#endif


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


/**
 * Puts the current timestamp in '[YYYY-MM-DD_hh:mm:ss]' format into the given
 * buffer.
 * @return A pointer to a null terminated timestamp string, ready for printing
 */
char* timestamp() {
  static char timestamp[22];

  time_t rawtime = time(NULL);
  struct tm* timeinfo = localtime(&rawtime);

  strftime(timestamp, 22, "[%F_%T]", timeinfo);
  return timestamp;
}