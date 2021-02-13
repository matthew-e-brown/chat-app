#ifndef __CLIENT_CONSTANTS__
#include "./constants.h"
#endif
#ifndef __GLOBAL__
#include "../global.c"
#endif

#include <ctype.h>
#include <ncurses.h>
#include <string.h>

extern int server_sock;
extern WINDOW *text_window, *chat_window;
extern unsigned int cur_x, cur_y, msg_l;
extern char current_message[MAX_MSG_LEN];
extern char my_username[MAX_NAME_LEN];


void send_message();
void display_own_message(char* str, int whisper, char* dest);


/**
 * Reads a character from stdin within the context of text_window. Handles all
 * the writing to the current_message buffer and calling the send_message
 * function.
 */
void handle_input() {

  // Strategy: **always** manipulate the current_message buffer directly, then
  // re-print the **whole** message. This will prevent a decouple between the
  // displayed message and what's in the buffer

  // >> get char

  int c = wgetch(text_window);
  if (c == ERR) return;

  // offset from prompt
  unsigned int o = strnlen(prompt_message, 64);
  // start of message in the form of x-pos
  unsigned int home = o + 2;
  // end pos of buffer on screen
  unsigned int end = home + msg_l;
  // msg_l before modifying the buffer: used to print ' ' over message
  unsigned int s_len = msg_l;

  getyx(text_window, cur_y, cur_x);

  int cur_idx = cur_x - home;          // current index into *buffer*
  int after_c = msg_l - cur_idx + 1;   // length of buffer behind the index

  switch (c) {
    // ---------------------------------------------->> Backspace and Delete
    case KEY_BACKSPACE:
      if (cur_x > home) {
        // >> Shift from current cursor into one before the current cursor
        memmove(
          &current_message[cur_idx - 1], &current_message[cur_idx], after_c
        );
        current_message[msg_l--] = '\0';
        cur_x -= 1;
      }
      break;
    case KEY_DC:
      if (cur_x < end) {
        // >> Shift from behind current cursor into current cursor
        memmove(
          &current_message[cur_idx], &current_message[cur_idx + 1], after_c
        );
        current_message[msg_l--] = '\0';
      }
      break;
    // ---------------------------------------------->> Left and Right
    case KEY_LEFT: if (cur_x > home) cur_x -= 1; break;
    case KEY_RIGHT: if (cur_x < end) cur_x += 1; break;
    // ---------------------------------------------->> Home and End
    case KEY_HOME: cur_x = home; break;
    case KEY_END: cur_x = end; break;
    // ---------------------------------------------->> Enter
    case 10: // Ascii 'enter' value
    case KEY_ENTER: // Occasional NCURSES "enter from numeric keypad" value
      // >> Send message
      send_message();
      memset(current_message, 0, MAX_MSG_LEN);
      cur_x = home;
      msg_l = 0;
      break;
    // ---------------------------------------------->> All others
    default:
      if (isprint(c)) {
        // >> They've entered a character
        if (msg_l >= MAX_MSG_LEN) {
          // Message too long!!

        } else if (cur_x < end) {
          // We're not at end of string: will need to shunt buffer forwards
          memmove(
            &current_message[cur_idx + 1], &current_message[cur_idx], after_c
          );
          current_message[cur_idx] = c;
          msg_l += 1;
          cur_x += 1;
        } else {
          // At end of string: can simply append to buffer
          current_message[msg_l++] = c;
          cur_x += 1;
        }
      }
      break;
  }

  // >> redraw the buffer onto the screen

  // start by clearing the previous characters
  wmove(text_window, cur_y, home);
  wprintw(text_window, "%*s", s_len, "");

  // then print the real buffer and move back
  wmove(text_window, cur_y, home);
  wprintw(text_window, current_message);
  wmove(text_window, cur_y, cur_x);

  wrefresh(text_window);
}


void send_message() {
  Packet message;

  char* split = strstr(current_message, string_splitter);
  if (split == NULL) {

    // >> Not a whisper
    split = strstr(current_message, command_marker);

    if (split == current_message) {
      // >> command, split was at the start
      strncpy(message.body, current_message + 1, MAX_MSG_LEN - 1);
      message.header = create_header(MSG_COMMAND, my_username, 0);

    } else {
      // >> broadcast
      strncpy(message.body, current_message, MAX_MSG_LEN);
      message.header = create_header(MSG_BROADCAST, my_username, 0);
      display_own_message(current_message, 0, NULL);
    }

  } else {
    // >> '::' was found

    // italics/ligatures may make this diagram a bit off ...
    //
    //    split split+2
    //    ____↓_↓_____________
    //    name::message here!!
    //
    // -> name = current_message
    //  -> length = split - current_message
    // -> message = split + 2
    //  -> length = MAX_MSG_LEN - (split + 2 - current_message)

    // >> find the lengths of both message halves

    size_t name_size = split - current_message;
    size_t msg_size = MAX_MSG_LEN - (split + 2 - current_message);

    // >> create temporary buffers to put the halves into, to avoid going over
    // >> the lengths of the chunks of current_message buffer

    char *temp_buff_name = calloc(MAX_NAME_LEN, sizeof(char));
    char *temp_buff_msg = calloc(MAX_MSG_LEN, sizeof(char));

    strncpy(temp_buff_name, current_message, name_size);
    strncpy(temp_buff_msg, split + 2, msg_size);

    // >> null out the rest of the string, then send from start of string
    message.header = create_header(MSG_WHISPER, my_username, temp_buff_name);
    strncpy(message.body, temp_buff_msg, MAX_MSG_LEN);

    display_own_message(temp_buff_msg, 1, temp_buff_name);

    free(temp_buff_name);
    free(temp_buff_msg);
  }

  message.header.packet_count = 1;
  message.header.packet_index = 0;

  send(server_sock, &message, sizeof message, 0);
}


/**
 * Display the user's own message in the chat screen. Does not send to server.
 * @param str The message to display
 * @param whisper 1/0 for if the message was whispered or not
 * @param dest The person who was whispered to; NULL if whisper == 0
 */
void display_own_message(char* str, int whisper, char* dest) {
  wattr_on(chat_window, COLOR_PAIR(OWN_PAIR), NULL);
  switch (whisper) {
    case 0: wprintw(chat_window, "You said: "); break;
    case 1: wprintw(chat_window, "You whispered to %s: ", dest); break;
  }
  wattr_off(chat_window, COLOR_PAIR(OWN_PAIR), NULL);
  wprintw(chat_window, "%s\n", str);
  wrefresh(chat_window);
}


/**
 * Writes a packet to the chat_window screen.
 * @param message A pointer to the message to display
 */
void display_message(Packet *message) {
  int pair = -1;
  char buff[32];
  size_t s = sizeof buff;

  switch (message->header.message_type) {
    case MSG_BROADCAST:
      snprintf(buff, s, "%s says: ", message->header.sender_name);
      pair = BROADCAST_PAIR;
      break;
    case MSG_WHISPER:
      snprintf(buff, s, "%s whispered to you: ", message->header.sender_name);
      pair = WHISPER_PAIR;
      break;
    case MSG_ANNOUNCE:
      snprintf(buff, s, "server says: ");
      pair = SERVER_PAIR;
      break;
    case MSG_ACK_WITH_ERR:
      snprintf(buff, s, "server responded with error: ");
      pair = SERVER_PAIR;
      break;
  }

  if (pair != -1) wattr_on(chat_window, COLOR_PAIR(pair), NULL);
  wprintw(chat_window, buff);
  if (pair != -1) wattr_off(chat_window, COLOR_PAIR(pair), NULL);

  wprintw(chat_window, "%s\n", message->body);
  wrefresh(chat_window);
}