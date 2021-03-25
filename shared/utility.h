#ifndef __GLOBAL_FUNCTIONS__
#define __GLOBAL_FUNCTIONS__

int setup_epoll(int* epoll_fd, int files[], int count);
char* timestamp();

#endif