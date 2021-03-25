/**
 * COIS-4310H: Chat App
 *
 * @name:         Chat App -- Input functions
 *
 * @author:       Matthew Brown, #0648289
 * @date:         February 1st to February 12th, 2021  
 *                March 1st to March 9th, 2021
 *
 * @purpose:      Holds code for manipulating the main message buffer and all
 *                the helper functions that requires. Also includes code for
 *                properly updating the scrolling pad.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ctype.h>
#include <curses.h>

#include "../shared/constants.h"

#include "./constants.h"
#include "./input.h"

// -- Function headers

static unsigned int get_position_in_line();
static void goto_line_start();
static void goto_line_end();

static inline int line_end(char c) { return (c == '\n' || c == '\0'); }


void get_cursor_pos(unsigned int buff_pos, unsigned int* y, unsigned int* x) {
  unsigned int i = 0;
  *y = 0;
  *x = 0;

  for (i = 0; i < buff_pos; i++) {
    if (current_message[i] == '\0') break;
    else if (current_message[i] == '\n') {
      *x = 0;
      *y += 1;
    } else {
      *x += 1;
    }
  }
}


void update_pad(unsigned int cur_y, unsigned int cur_x) {
  static unsigned int ystart = 0, xstart = 0;  // Start position of pad
  unsigned int pad_h = (LINES - 2) - (LINES - 4) + 1;
  unsigned int pad_w = (COLS - 3) - (2 + prompt_length + 1) + 1;

  // Cursor is off the bottom
  while (cur_y >= ystart + pad_h) ystart += 1;
  // Cursor is off the top
  while (cur_y < ystart) ystart -= 1;
  // Cursor is off the right
  while (cur_x >= xstart + pad_w) xstart += 1;
  // Cursor is off the left
  while (cur_x < xstart) xstart -= 1;

  wmove(text_window, cur_y, cur_x);
  prefresh(text_window,
    ystart, xstart,
    LINES - 4, 2 + prompt_length + 1,
    LINES - 2, COLS - 3
  );
}


int handle_input() {
  unsigned int i;

  // Strategy: **always** manipulate the current_message buffer directly, then
  // re-print the **whole** message. This will prevent a decouple between the
  // displayed message and what's in the buffer

  // >> Start by reading character
  int c = wgetch(text_window);
  if (c == ERR) return 0;

  // Used for if-statements and memory management
  size_t length = strlen(current_message);  // Current length of the string
  size_t to_end = length - pos;             // Size of message after the cursor

  switch (c) {
    // ------------------------------------------------------>> Movement
    case KEY_LEFT: if (pos > 0) pos -= 1; break;
    case KEY_RIGHT: if (pos < length) pos += 1; break;
    case KEY_HOME: goto_line_start(); break;
    case KEY_END: goto_line_end(); break;
    case KEY_UP: {
      unsigned int line_dist = get_position_in_line();
      // >> Move to start of current line
      goto_line_start();
      // >> If we didn't hit the start
      if (pos > 0) {
        // >> Move to the end of the previous line
        pos -= 1;
        // >> Move to the start of the NEW current line
        goto_line_start();
        // >> Move forwards by line_dist amount
        for (i = 0; i < line_dist - 1; i++) {
          if (line_end(current_message[pos])) break;
          pos += 1;
        }
      }
    } break;
    case KEY_DOWN: {
      unsigned int line_dist = get_position_in_line();
      // >> Move to the end of the current line
      goto_line_end();
      // >> If this isn't the last line...
      if (current_message[pos] != '\0') {
        // >> Move to the start of the next one
        pos += 1;
        // >> Move forwards line_dist amount, stopping at end of line or string
        //    No - 1 this time because we need to include the \n from above
        for (i = 0; i < line_dist; i++) {
          if (line_end(current_message[pos])) break;
          pos += 1;
        }
      }
    } break;

    // ------------------------------------------------------>> Deletion

    case KEY_BACKSPACE:
      if (pos > 0) {
        memmove(current_message + pos - 1, current_message + pos, to_end);
        current_message[length - 1] = '\0';
        pos -= 1;
      }
      break;
    case KEY_DC:
      if (pos < length) {
        memmove(current_message + pos, current_message + pos + 1, to_end);
        current_message[length - 1] = '\0';
      }
      break;

    // ------------------------------------------------------>> Enter(s)

    // Swap these two cases to change CTRL+ENTER / ENTER for sending and
    // new-lining

    // CTRL+Enter (in nonl mode)
    case '\n': c = '\n'; goto insert;
    // Enter (in nonl mode)
    case '\r': return 1; // send message

    // ------------------------------------------------------>> All others

    default:
      if (isprint(c)) {
        insert:;
        // Triggers at 0x0fff chars, to keep room for null term
        if (pos >= MSG_BUFF - 1) {
          // >> Message too long!
        } else {
          // >> If we aren't at the end, shunt the buffer forwards before
          //    putting the character in place
          if (pos < length)
            memmove(current_message + pos + 1, current_message + pos, to_end);
          current_message[pos++] = c;
        }
      }
      break;
  }

  // >> Redraw Buffer

  unsigned int y, x;
  get_cursor_pos(pos, &y, &x);  // Get cursor XY position before buffer reprint

  werase(text_window);                    // Clear pad
  wprintw(text_window, current_message);  // Print whole buffer

  update_pad(y, x);             // Move pad as required and redraw

  return 0;
}


/**
 * Get distance from 'pos' to the start of the current line in the current
 * buffer.
 * @return The distance
 */
static unsigned int get_position_in_line() {
  // >> Start at 'pos' and rewind until we hit either \n or the start of the
  //    string

  unsigned int spot = 0;    // Counter of chars before \n or pos=0
  unsigned int tpos = pos;  // temp pos counter, avoid chainging main pos

  // If we are at the end of the current line (over-top of its \n character),
  // need to go back one more space. We can't use a do {} while in this case
  // because we only want the extra count *if* we are starting on the new-line
  if (current_message[pos] == '\n') {
    tpos -= 1;
    spot += 1;
  }

  while (tpos > 0 && current_message[tpos] != '\n') {
    tpos -= 1;
    spot += 1;
  }

  return spot;
}


/**
 * Move the cursor to the start of the current line.
 */
static void goto_line_start() {
  // Use a temporary signed integer to allow value to dip below zero for one
  // loop iteration, then be ticked back up again to zero
  if (pos > 0) {
    int c = pos;
    do {
      c -= 1;
    } while (c >= 0 && !line_end(current_message[c]));
    pos = c + 1;
  }
}


/**
 * Move the cursor to the end of the current line.
 */
static void goto_line_end() {
  while (!line_end(current_message[pos])) pos += 1;
}