#ifndef __SERVER_COMMANDS__
#define __SERVER_COMMANDS__

#include "../shared/messaging.h"

// Function pointer definition for command functions
typedef int (*command_ptr)(Message*);

/**
 * Returns the function to be used for the command string passed.
 * @param string The string to check for a matching command to
 * @return A pointer to the function to call
 */
command_ptr find_command(const char string[]);

/**
 * COMMAND: Lists all users on the server.
 * @param dest The message to place the response in.
 * @return A status code.
 */
int command_who(Message* dest);

#endif