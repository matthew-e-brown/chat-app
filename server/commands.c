#define __SERVER_COMMANDS__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

#ifndef __GLOBAL_CONSTANTS__
#include "../shared/constants.h"
#endif
#ifndef __GLOBAL_MESSAGING__
#include "../shared/messaging.c"
#endif
#ifndef __SERVER_CONSTANTS__
#include "./constants.h"
#endif

extern User users[CONN_LIMIT];
extern Thread threads[CONN_LIMIT];
extern pthread_mutex_t users_lock;
extern pthread_mutex_t threads_lock;


// Function pointer definition for command functions
typedef int (*command_ptr)(Message*);

// -- Headers for command functions

int command_who(Message* dest);

// -- Utility struct for searching
static struct command_pair {
  // name[] needs to be long enough to hold the longest command name. change as
  // needed.
  const char name[4];      // The commands name; the string to pass
  const command_ptr func;  // Pointer
} commands[] = {

  { "who", &command_who }

};

// -- Functions

/**
 * Returns the function to be used for the command string passed.
 * @param string The string to check for a matching command to
 * @return A pointer to the function to call
 */
command_ptr find_command(const char string[]) {
  int i;

  for (i = 0; i < NUM_ELEMS(commands); i++) {
    if (strcmp(commands[i].name, string) == 0) return commands[i].func;
  }

  return NULL;
}

// -- Commands

/**
 * COMMAND: Lists all users on the server.
 * @param dest The message to place the response in.
 * @return A status code.
 */
int command_who(Message* dest) {
  int i, j;

  const char pre[] = "All users: ";

  char** chunks; // Array of strings to join at the end
  size_t* sizes; // Array of chunk sizes

  pthread_mutex_lock(&users_lock);
  pthread_mutex_lock(&threads_lock);

  // >> Count number of users for proper allocation
  int count = 0;
  for (i = 0; i < CONN_LIMIT; i++) if (threads[i].in_use) count++;
  if (count == 0) return -1; // Somehow, nobody is connected but command ran??

  // Each entry in array is a pointer to one chunk of the output
  chunks = malloc(sizeof(char*) * count);
  sizes = malloc(sizeof(size_t) * count);

  j = 0; // Secondary counter; the number of used threads run into so far
  for (i = 0; i < CONN_LIMIT; i++) {
    if (threads[i].in_use) {

      sizes[j] = strlen(threads[i].user->username);
      if (j + 1 < count) sizes[j] += 2; // include room for ", " on not-last

      chunks[j] = malloc(sizes[j]);

      if (j + 1 < count) sprintf(chunks[j], "%s, ", threads[i].user->username);
      else sprintf(chunks[j], "%s", threads[i].user->username);

      j += 1;
    }
  }

  pthread_mutex_unlock(&threads_lock);
  pthread_mutex_unlock(&users_lock);

  // >> Count final message size
  size_t total_size;
  for (j = 0; j < count; j++) total_size += sizes[j];

  size_t offset = strlen(pre); // amount copied so far

  // >> Allocate final chunk, copy prefix in
  dest->body = malloc(total_size + offset);
  memcpy(dest->body, pre, offset);

  for (j = 0; j < count; j++) {
    memcpy(dest->body + offset, chunks[j], sizes[j]);
    offset += sizes[j];
  }

  // >> Set the rest of the metadata for the message
  dest->type = SRV_RESPONSE;
  dest->size = strlen(dest->body);
  memset(dest->sender_name, 0, USERNAME_MAX);
  memset(dest->receiver_name, 0, USERNAME_MAX);

  // >> Free up used buffers
  for (j = 0; j < count; j++) free(chunks[j]);
  free(chunks);
  free(sizes);

  return 0;
}