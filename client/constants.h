#define __CLIENT_CONSTANTS__

#define MSG_BUFF 0x1000   // The maximum message size: how big the buffer is

// -- Constant strings used by client app

const char string_splitter[] = "::";  // Goes between [name]::[whisper]
const char command_marker[] = "";     // Goes before commands

// --->> When you change string_splitter, make sure to update the welcome
//       message!

const char prompt_message[] = "Enter a message: >>";  // Displayed in prompt
const size_t prompt_length = 19;                      // Length without \0

// Displayed at the start of the program
const char welcome_message[] =
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
  "    /bye\n"
  "      -> Leave the server and close the app.\n"
  "    /who\n"
  "      -> See all currently connected users.";


// -- Color pairs used by Curses

#define CPAIR_BROADCAST   ((short)(0x0001))
#define CPAIR_WHISPER     ((short)(0x0002))
#define CPAIR_OWN_MSG     ((short)(0x0003))
#define CPAIR_NOTICE      ((short)(0x0004))
#define CPAIR_ERROR       ((short)(0x0005))