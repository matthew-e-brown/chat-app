/**
 * COIS-4310H: Chat App
 *
 * @name:         Chat App -- Client constants
 *
 * @author:       Matthew Brown, #0648289
 * @date:         February 1st to February 12th, 2021  
 *                March 1st to March 9th, 2021
 *
 * @purpose:      Constant variables and macro definitions used by different
 *                parts of the client app.
 *
 */


#ifndef __CLIENT_CONSTANTS__
#define __CLIENT_CONSTANTS__

#define MSG_BUFF 0x1000   // The maximum message size: how big the buffer is

// -- Constant strings used by client app

#define STRING_SPLIT "::"
#define COMMAND_MARK "/"

// Color pairs used by Curses:

#define CPAIR_BROADCAST   ((short)(0x0001))
#define CPAIR_WHISPER     ((short)(0x0002))
#define CPAIR_OWN_MSG     ((short)(0x0003))
#define CPAIR_NOTICE      ((short)(0x0004))
#define CPAIR_ERROR       ((short)(0x0005))

// -- Global variable *declarations*

extern WINDOW* chat_window;  // The window where messages are displayed
extern WINDOW* text_window;  // The window the user types into

extern char current_message[MSG_BUFF];  // Current message being typed
extern char my_username[USERNAME_MAX];  // User's username
extern unsigned int pos;                // Tracked cursor pos in the buffer

// Displayed at the start of the program
extern const char help_message[];

extern const char prompt_message[];  // Displayed in prompt
extern const size_t prompt_length;   // Length without \0

#endif