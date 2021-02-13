#define __CLIENT_CONSTANTS__

#define MESSAGE_COUNT 64

#define SERVER_PAIR 1
#define WHISPER_PAIR 2
#define BROADCAST_PAIR 3
#define OWN_PAIR 4

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