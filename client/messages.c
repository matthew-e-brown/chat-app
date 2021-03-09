/**
 * COIS-4310H: Chat App
 *
 * @name:         Chat App -- Messaging functions
 *
 * @author:       Matthew Brown, #0648289
 * @date:         February 1st to February 12th, 2021  
 *                March 1st to March 9th, 2021
 *
 * @purpose:      Holds the functions required for taking the current message
 *                buffer and sending it to the server. Also holds functions for
 *                displaying message on the screen.
 *
 */


#define __CLIENT_MESSAGES__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <curses.h>

#ifndef __GLOBAL_CONSTANTS__
#include "../shared/constants.h"
#endif
#ifndef __GLOBAL_MESSAGING__
#include "../shared/messaging.c"
#endif
#ifndef __CLIENT_CONSTANTS__
#include "./constants.h"
#endif
#ifndef __CLIENT_INPUT__
#include "./input.c"
#endif

extern WINDOW* chat_window;
extern WINDOW* text_window;

extern char current_message[MSG_BUFF];
extern char my_username[USERNAME_MAX];
extern unsigned int pos;


/**
 * Turns the raw text from current_message into a Message object, ready to be
 * sent.
 * @return The message in the form of the Message struct
 */
Message parse_buffer() {
  // >> On run, get the sizes of the splitters so that we can calculate offsets
  size_t SPLIT_SIZE = strlen(STRING_SPLIT);
  size_t CMARK_SIZE = strlen(COMMAND_MARK);

  Message result;

  char* split_ptr = strstr(current_message, STRING_SPLIT);

  if (split_ptr != NULL) {
    // >> STRING_SPLIT found
    char* receiver_name = current_message;
    char* message_start = split_ptr + SPLIT_SIZE;

    *split_ptr = '\0'; // null terminate the username

    if (strlen(receiver_name) >= USERNAME_MAX) {
      const char* err =
        "That username is too long to be a valid whisper targer.\n";

      // >> The username they typed is too long
      result.type = USR_ERROR;
      memset(result.sender_name, 0, USERNAME_MAX);
      memset(result.receiver_name, 0, USERNAME_MAX);

      result.size = strlen(err);
      result.body = calloc(result.size, 1);
      memcpy(result.body, err, result.size);

    } else {
      // >> Ready to send

      result.type = MSG_WHISPER;
      memcpy(result.sender_name, my_username, USERNAME_MAX);

      // >> Include the new '\0' byte in the receiver name
      memcpy(result.receiver_name, receiver_name, strlen(receiver_name) + 1);

      result.size = strlen(message_start);
      result.body = calloc(result.size, 1);
      memcpy(result.body, message_start, result.size);

    }

  } else if (strstr(current_message, COMMAND_MARK) == current_message) {
    // >> COMMAND_MARK was found at the start
    result.type = MSG_COMMAND;

    memset(result.receiver_name, 0, USERNAME_MAX);
    memcpy(result.sender_name, my_username, USERNAME_MAX);

    result.size = strlen(current_message) - CMARK_SIZE;
    result.body = calloc(result.size, 1);
    memcpy(result.body, current_message + CMARK_SIZE, result.size);

  } else {
    // >> STRING_SPLIT not found, is a regular message
    result.type = MSG_BROADCAST;

    memset(result.receiver_name, 0, USERNAME_MAX);
    memcpy(result.sender_name, my_username, USERNAME_MAX);

    result.size = strlen(current_message);
    result.body = calloc(result.size, 1);
    memcpy(result.body, current_message, result.size);
  }

  return result;
}


/**
 * Takes a message and puts it on the screen with nice colours.
 * @param message The message to display
 */
void display_message(Message message) {
  short pair = -1;
  char preface[48];

  switch (message.type) {
    case MSG_BROADCAST:
      pair = CPAIR_BROADCAST;
      sprintf(preface, "%s says:\n", message.sender_name);
      break;
    case MSG_WHISPER:
      pair = CPAIR_WHISPER;
      sprintf(preface, "%s whispers to you:\n", message.sender_name);
      break;
    case SRV_ANNOUNCE:
      pair = CPAIR_NOTICE;
      sprintf(preface, "Server says:\n");
      break;
    case SRV_RESPONSE:
      pair = CPAIR_NOTICE;
      sprintf(preface, "Server replied with:\n");
      break;
    case SRV_ERROR:
      pair = CPAIR_ERROR;
      sprintf(preface, "Something went wrong! (Server-side) error:\n");
      break;
    case USR_ERROR:
      pair = CPAIR_ERROR;
      sprintf(preface, "Something went wrong! (User) error:\n");
      break;
    default:
      wprintw(chat_window, "An unknown message type was received.\n");
      wrefresh(chat_window);
      return;
  }

  if (has_colors()) wattr_on(chat_window, COLOR_PAIR(pair), NULL);
  wprintw(chat_window, "%s %s", timestamp(), preface);
  if (has_colors()) wattr_off(chat_window, COLOR_PAIR(pair), NULL);

  wprintw(chat_window, "%s\n\n", message.body);
  wrefresh(chat_window);
}


/**
 * Wrapper for `display_message`. Sets sender field to "you" (etc.) before
 * forwarding for display.
 * @param message The message to display
 */
void display_own_message(Message message) {

  // Double check that this is our own message before actually manipulating it.
  // IE, if the client spits out an error, we don't want to display as if we
  // said it, so we forward that to the main display_message function

  if (
    strcmp(message.sender_name, my_username) == 0 &&
    (message.type == MSG_WHISPER || message.type == MSG_BROADCAST)
  ) {

    if (has_colors()) wattr_on(chat_window, COLOR_PAIR(CPAIR_OWN_MSG), NULL);

    switch (message.type) {
      case MSG_WHISPER:
        wprintw(chat_window, "%s You whispered to %s:\n",
          timestamp(), message.receiver_name);
        break;
      case MSG_BROADCAST:
        wprintw(chat_window, "%s You said:\n",
          timestamp());
        break;
    }

    if (has_colors()) wattr_off(chat_window, COLOR_PAIR(CPAIR_OWN_MSG), NULL);

    wprintw(chat_window, "%s\n\n", message.body);
    wrefresh(chat_window);

  } else if (message.type != MSG_COMMAND) {
    // Don't want to display anything for commands
    display_message(message);
  }
}