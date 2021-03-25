#ifndef __GLOBAL_FUNCTIONS__
#define __GLOBAL_FUNCTIONS__

/**
 * Sets up a bunch of file listeners on a specific epoll fd.
 * @param epoll_fd A pointer to the epoll_fd to set
 * @param files All the file descriptors to listen to on epoll
 * @param count The count of files in the files array passed
 * @return 0 on success, -1 on epoll_create failure, and index of files array on
 * any other failure (1 indexed, to avoid collision w/ 0)
 */
int setup_epoll(int* epoll_fd, int files[], int count);

/**
 * Puts the current timestamp in '[YYYY-MM-DD hh:mm:ss]' format into the given
 * buffer.
 * @return A pointer to a null terminated timestamp string, ready for printing
 */
char* timestamp();

#endif