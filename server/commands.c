/**
 * COIS-4310H: Chat App
 *
 * @name:         Chat App -- Command functions and definitions
 *
 * @author:       Matthew Brown, #0648289
 * @date:         February 1st to February 12th, 2021  
 *                March 1st to March 9th, 2021
 *
 * @purpose:      The file holds definitions for the commands of the server.
 *                Each command is defined as a function that takes a pointer to
 *                a message struct as an argument and returns an int status
 *                code. The commands[] array holds a map between command names
 *                (as strings) and pointers to the right function.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

#include "../shared/constants.h"
#include "../shared/messaging.h"

#include "./constants.h"
#include "./commands.h"


// Utility struct for searching
static struct command_pair {
  // name[] needs to be long enough to hold the longest command name. change as
  // needed.
  const char name[4];      // The commands name; the string to pass
  const command_ptr func;  // Pointer
} commands[] = {

  { "who", &command_who }

};


// -- Helper functions

command_ptr find_command(const char string[]) {
  int i;

  for (i = 0; i < NUM_ELEMS(commands); i++) {
    if (strcmp(commands[i].name, string) == 0) return commands[i].func;
  }

  return NULL;
}

// -- Commands

int command_who(Message* dest) {
  int i, j;

  const char pre[] = "All users: ";

  char** chunks; // Array of strings to join at the end
  size_t* sizes; // Array of chunk sizes

  pthread_mutex_lock(&ut_lock);

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

  pthread_mutex_unlock(&ut_lock);

  // >> Count final message size
  size_t total_size = 0;
  for (j = 0; j < count; j++) total_size += sizes[j];

  size_t offset = strlen(pre); // amount copied so far

  // >> Allocate final chunk, copy prefix in
  dest->body = calloc(total_size + offset, 1);
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