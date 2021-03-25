#ifndef __CLIENT_MESSAGES__
#define __CLIENT_MESSAGES__

#include "../shared/messaging.h"


/**
 * Turns the raw text from current_message into a Message object, ready to be
 * sent.
 * @return The message in the form of the Message struct
 */
Message parse_buffer(const char buffer[]);

/**
 * Takes a message and puts it on the screen with nice colours.
 * @param message The message to display
 */
void display_message(Message message);

/**
 * Wrapper for `display_message`. Sets sender field to "you" (etc.) before
 * forwarding for display.
 * @param message The message to display
 */
void display_own_message(Message message);

#endif