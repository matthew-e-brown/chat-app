#ifndef __SERVER_COMMANDS__
#define __SERVER_COMMANDS__

#include "../shared/messaging.h"

typedef int (*command_ptr)(Message*);

int command_who(Message* dest);
command_ptr find_command(const char string[]);

#endif