#define __SERVER_FUNCTIONS__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

#ifndef __GLOBAL_CONSTANTS__
#include "../shared/constants.h"
#endif
#ifndef __SERVER_CONSTANTS__
#include "./constants.h"
#endif

extern Thread threads[CONN_LIMIT];
extern User users[CONN_LIMIT];
extern pthread_mutex_t threads_lock;
extern pthread_mutex_t users_lock;

/**
 * Get a pointer to a thread based on a username.
 * @param username The username to search for.
 * @return A pointer to that user's thread or NULL on not finding anything.
 */
Thread* get_thread_by_username(const char username[]) {
  int i;
  Thread* result;

  pthread_mutex_lock(&users_lock);
  pthread_mutex_lock(&threads_lock);

  for (i = 0; i <= CONN_LIMIT; i++) {
    if (i == CONN_LIMIT) { // Didn't get found
      result = NULL;
      break;
    }

    if (!threads[i].in_use) continue; // Don't need to check this one
    if (strncmp(threads[i].user->username, username, USERNAME_MAX) == 0) {
      // >> Found!!
      result = threads + i;
      break;
    }
  }

  pthread_mutex_unlock(&threads_lock);
  pthread_mutex_unlock(&users_lock);

  return result;
}