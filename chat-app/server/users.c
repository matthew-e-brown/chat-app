/**
 * COIS-4310H: Chat App
 *
 * @name:         users.c
 *
 * @author:       Matthew Brown; #0648289
 * @date:         February 1st to February 12th, 2021
 *
 * @purpose:      This file holds definitions for the types that the server uses
 *                to track connected users. It also brings with it a few helper
 *                functions for searching arrays for pointers to those users and
 *                the threads they may be stored in.
 *
 *                The Thread type is used to hold information a client-thread
 *                ( @see client_thread.c ) may need to perform its duties.
 *                Each thread gets one of these structs.
 */

#define __USERS__

#include <string.h>

// Include globals and constants (mostly for IDE highlighting, the ifndefs will
// protect them from re-including/looping during compilation)

#ifndef __GLOBAL__
#include "../global.c"
#endif
#ifndef __SERVER_CONSTANTS__
#include "./constants.h"
#endif


typedef struct user {
  int socket_fd;                // The FD of the user's socket
  char username[MAX_NAME_LEN];  // The user's name
} User;


typedef struct thread {
  unsigned char in_use; // T/F value for if the thread is in use or not
  pthread_t thread_id;  // The thread identifier
  int pipe_fd[2];       // The FD of the pipe this thread can send data through
  User* user;           // Pointer to the user this thread is responsible for
} Thread;


/**
 * Searches the provided array for a user based on their username
 * @param users Pointer to the start of the users array
 * @param arr_size How many users are in that array
 * @param username The name to search for
 * @returns Pointer to the found User, or NULL if nothing is found
 */
User* get_user_by_name(User* users, int arr_size, char* username) {
  int i;
  for (i = 0; i < arr_size; i++) {
    if (strncmp(users[i].username, username, MAX_NAME_LEN) == 0)
      return &users[i];
  }

  return NULL;
}


/**
 * Searches the provided array for a user based on their socket descriptor
 * @param users Pointer the to the start of the users array
 * @param arr_size How many users are in that array
 * @param socket_fd The socket file descriptor to look for
 * @returns Pointer to the found User, or NULL if nothing is found
 */
User* get_user_by_sock(User* users, int arr_size, int socket_fd) {
  int i;
  for (i = 0; i < arr_size; i++) {
    if (users[i].socket_fd == socket_fd) return &users[i];
  }

  return NULL;
}


/**
 * Searches the provided array for the thread which contains that user
 * @param threads The pointer to the start of the threads array
 * @param arr_size How many threads there are too search through
 * @param user The user pointer to search for
 * @returns Pointer to the found Thread, or NULL if nothing found
 */
Thread* get_thread_by_user(Thread* threads, int arr_size, User* user) {
  int i;
  for (i = 0; i < arr_size; i++) {
    if (threads[i].user == user) return &threads[i];
  }

  return NULL;
}