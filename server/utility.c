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
  Thread* result = NULL;

  for (i = 0; i <= CONN_LIMIT; i++) {
    // Didn't get found
    if (i == CONN_LIMIT) break;
    if (!threads[i].in_use) continue; // Don't need to check this one
    if (strncmp(threads[i].user->username, username, USERNAME_MAX) == 0) {
      // >> Found!!
      result = threads + i;
      break;
    }
  }

  return result;
}