/**
 * COIS-4310H: Chat App
 *
 * @name:         Chat App -- Server constants
 *
 * @author:       Matthew Brown, #0648289
 * @date:         February 1st to February 12th, 2021  
 *                March 1st to March 9th, 2021
 *
 * @purpose:      Constant variables, types, and macro definitions used by
 *                different parts of the server.
 *
 */

#ifndef __SERVER_CONSTANTS__
#define __SERVER_CONSTANTS__

#include <pthread.h>
#include <sys/socket.h>

#include "../shared/constants.h"

#define CONN_LIMIT 8     // The maximum connected users at a time

// -- Global utility structs

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
  pthread_t id;          // PThread identifier for library functions
  int pipe_fd[2];        // The FD this pipe uses to receive data from main
  User* user;            // Pointer to the user this thread is responsible for
} Thread;

// -- Global variable *declarations*

extern User users[CONN_LIMIT];         // Holds all users' metadata
extern Thread threads[CONN_LIMIT];     // Holds all client thread metadata

extern pthread_mutex_t threads_lock;   // Locks access to 'threads'
extern pthread_mutex_t users_lock;     // Locks access to 'users'

extern int master_sock;                // Bound listener socket
extern struct sockaddr_in master_addr; // Address of the bound socket
extern socklen_t master_addr_size;     // Used for bind

extern int master_pipe[2];  // Pipe for threads to message back to main thread

#endif