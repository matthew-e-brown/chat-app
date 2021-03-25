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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <curses.h>

#include "../shared/constants.h"
#include "../shared/messaging.h"
#include "../shared/utility.h"

#include "./constants.h"
#include "./messages.h"
#include "./input.h"


Message parse_buffer(const char buffer[]) {
  // >> On run, get the sizes of the splitters so that we can calculate offsets
  size_t SPLIT_SIZE = strlen(STRING_SPLIT);
  size_t CMARK_SIZE = strlen(COMMAND_MARK);

  Message result;

  char* split_ptr = strstr(buffer, STRING_SPLIT);

  if (split_ptr != NULL) {
    // >> STRING_SPLIT found
    const char* receiver_name = buffer;
    char* message_start = split_ptr + SPLIT_SIZE;

    *split_ptr = '\0'; // null terminate the username

    if (strlen(receiver_name) >= USERNAME_MAX) {
      const char* err =
        "That username is too long to be a valid whisper target.\n";

      // >> The username they typed is too long
      result.type = USR_ERROR;
      memset(result.sender_name, 0, USERNAME_MAX);
      memset(result.receiver_name, 0, USERNAME_MAX);

      result.size = strlen(err) + 1;
      result.body = calloc(result.size, 1);
      memcpy(result.body, err, result.size);

    } else {
      // >> Ready to send

      result.type = MSG_WHISPER;
      memcpy(result.sender_name, my_username, USERNAME_MAX);

      // >> Include the new '\0' byte in the receiver name
      memcpy(result.receiver_name, receiver_name, strlen(receiver_name) + 1);

      result.size = strlen(message_start) + 1;
      result.body = calloc(result.size, 1);
      memcpy(result.body, message_start, result.size);

    }

  } else if (strstr(buffer, COMMAND_MARK) == buffer) {
    // >> COMMAND_MARK was found at the start
    result.type = MSG_COMMAND;

    memset(result.receiver_name, 0, USERNAME_MAX);
    memcpy(result.sender_name, my_username, USERNAME_MAX);

    result.size = strlen(buffer) - CMARK_SIZE + 1;
    result.body = calloc(result.size, 1);
    memcpy(result.body, buffer + CMARK_SIZE, result.size);

  } else {
    // >> STRING_SPLIT not found, is a regular message
    result.type = MSG_BROADCAST;

    memset(result.receiver_name, 0, USERNAME_MAX);
    memcpy(result.sender_name, my_username, USERNAME_MAX);

    result.size = strlen(buffer) + 1;
    result.body = calloc(result.size, 1);
    memcpy(result.body, buffer, result.size);
  }

  return result;
}


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