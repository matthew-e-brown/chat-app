#define __SERVER_CONSTANTS__

#include <pthread.h>

#ifndef __GLOBAL_CONSTANTS__
#include "../shared/constants.h"
#endif

#define CONN_LIMIT 8


/**
 * Internal representation of a user; all it needs to store is the socket
 * they're connected to and the name.
 */
typedef struct user {
  int socket_fd;                // The FD of the user's socket
  char username[USERNAME_MAX];  // The user's username
} User;


/**
 * Struct for metadata about each thread.
 */
typedef struct server_thread {
  unsigned char in_use;  // 0/1 value for if the thread is currently being used
  pthread_t id;   // PThread identifier for library functions
  int pipe_fd[2];        // The FD this pipe uses to receive data from main
  User* user;            // Pointer to the user this thread is responsible for
} Thread;