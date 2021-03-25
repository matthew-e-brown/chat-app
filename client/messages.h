#ifndef __CLIENT_MESSAGES__
#define __CLIENT_MESSAGES__

#include "../shared/messaging.h"

Message parse_buffer(const char buffer[]);
void display_message(Message message);
void display_own_message(Message message);

#endif