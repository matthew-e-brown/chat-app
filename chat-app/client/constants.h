/**
 * COIS-4310H: Chat App
 *
 * @name:         constants.h
 *
 * @author:       Matthew Brown, #0648289
 * @date:         February 1st to February 12, 2021
 *
 * @purpose:      Holds constants that are used by functions within the client
 *                context.
 *
 */

#define __CLIENT_CONSTANTS__

// The color pair indecies used by NCURSES

#define SERVER_PAIR 1
#define WHISPER_PAIR 2
#define BROADCAST_PAIR 3
#define OWN_PAIR 4

// Things to search for in the user's message for certain types of messages

const char* string_splitter = "::";   // between name::message
const char* command_marker = "/";     // before /commands

// -->> Remember to change the help message if you update the above two!!

const char* welcome_message =
  "Hello! Welcome to the app. Type your messages in one of the following "
  "formats:\n"
  "\n"
  "    [message]\n"
  "      -> Broadcast your message to all connected users.\n"
  "    [name]::[message]\n"
  "      -> Whisper your message to [name].\n"
  "    /[command]\n"
  "      -> Run a command.\n"
  "\n"
  "Commands are as follows:\n"
  "    /who\n"
  "      -> See all currently connected users.";

const char* prompt_message = "Enter a message: >> ";