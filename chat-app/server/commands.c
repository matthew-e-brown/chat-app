/**
 * COIS-4310H: Chat App
 *
 * @name:         commands.c
 *
 * @author:       Matthew Brown; #0648289
 * @date:         February 1st to February 12th, 2021
 *
 * @purpose:      This file holds the functions for each of the / commands that
 *                the user can ask of the server. Alongside the function
 *                definitions are a type definition and helper function to get
 *                the pointer to the right function.
 *
 * @example:      The user sends "who" in a MSG_COMMAND packet. The helper
 *                function then determines which function, if any, the server
 *                must execute to perform that command. All command functions
 *                accept a pointer to a Packet, into the body of which they copy
 *                their response. This is shown by the typedef of the command_pt
 *                type.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

#ifndef __GLOBAL__
#include "../global.c"
#endif
#ifndef __SERVER_CONSTANTS__
#include "./constants.h"
#endif
#ifndef __USERS__
#include "./users.c"
#endif


extern User connected_users[MAX_CONNECTED_USERS];    // all users
extern pthread_mutex_t users_lock;                   // lock for users
extern Thread client_threads[MAX_CONNECTED_USERS];   // all threads
extern pthread_mutex_t thread_lock;                  // lock for threads

// the longest length commands strings can be
#define MAX_COMMAND_LENGTH 8

// type of funcion pointers for commands
typedef int (*command_pt)(Packet* message);

// -- Command function headers

int command_who(Packet* message);

// Struct list to hold all the commands linked with their names

struct command_pair {
  const char name[MAX_COMMAND_LENGTH];
  const command_pt func;
} commands[] = {

  { "who", &command_who } // link "who" with the pointer to the function

};

/**
 * Returns the pointer to the function that matches what the user typed
 * @param msg The message to analyze for /[command] syntax
 * @returns a pointer to the function that will run the command
 */
command_pt get_command(char* msg) {
  unsigned int i;
  for (i = 0; i < NUM_ELEMS(commands); i++) {
    if (strncmp(commands[i].name, msg, MAX_COMMAND_LENGTH) == 0) {
      return commands[i].func;
    }
  }
  return NULL;
}


/**
 * Copies all usernames in a comma separated list into the body of the provided
 * Packet.
 * @param message a pointer to the message whose body is to be copied into
 * @returns a 0 on success and a 1 on failure
 */
int command_who(Packet* message) {
  int i;
  int err = 0;
  // every name + ', ' for each except last
  char* buffer;
  size_t buff_size = MAX_NAME_LEN * MAX_CONNECTED_USERS +
    2 * (MAX_CONNECTED_USERS - 1);

  if (buff_size >= MAX_MSG_LEN) {
    // If the global constants of the server have been setup incorrectly, it's
    // possible that all the comma separated names won't fit in a buffer
    // properly. We do this just in case. In future version of the app, this
    // will just mean the command gets split up into mulitple packets
    buffer = calloc(MAX_MSG_LEN, sizeof(char));
    strncpy(
      buffer, "SERVER CONFIG ERROR: Names do not fit in message size.",
      MAX_MSG_LEN
    );
    err = 1;
  } else {
    buffer = calloc(buff_size, sizeof(char));

    pthread_mutex_lock(&thread_lock);

    // count connected users first to get correct comma offsetting
    int connected = 0;
    for (i = 0; i < MAX_CONNECTED_USERS; i++) {
      if (client_threads[i].in_use) connected += 1;
    }

    // buffer offset
    int o = 0;

    pthread_mutex_lock(&users_lock);

    for (i = 0; i < MAX_CONNECTED_USERS; i++) {
      if (client_threads[i].in_use) {
        // check how long the name length is
        size_t nl = strnlen(client_threads[i].user->username, MAX_NAME_LEN);
        strncpy(buffer + o, client_threads[i].user->username, buff_size - o);
        o += nl;
        if (i < connected - 1) {
          strncpy(buffer + o, ", ", buff_size - o);
          o += 2;
        }
      }
    }

    pthread_mutex_unlock(&users_lock);
    pthread_mutex_unlock(&thread_lock);
  }

  strncpy(message->body, buffer, MAX_MSG_LEN);
  free(buffer);

  return err;
}