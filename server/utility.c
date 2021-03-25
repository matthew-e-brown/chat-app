/**
 * COIS-4310H: Chat App
 *
 * @name:         Chat App -- Server utility functions
 *
 * @author:       Matthew Brown, #0648289
 * @date:         February 1st to February 12th, 2021  
 *                March 1st to March 9th, 2021
 *
 * @purpose:      This file holds functions used by different components of the
 *                server in multiple other places.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

#include "../shared/constants.h"

#include "./constants.h"
#include "./utility.h"


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