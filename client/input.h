#ifndef __CLIENT_INPUT__
#define __CLIENT_INPUT__

/**
 * Gets the (x, y) position within the pad for the current cursor from the 1-d
 * index into the message buffer.
 * @param buff_pos The index into the current message buffer to check y,x for
 * @param y Pointer to the y value to set
 * @param x Pointer to the x value to set
 */
void get_cursor_pos(unsigned int buff_pos, unsigned int* y, unsigned int* x);

/**
 * Reads user input from stdin within the context of the text_message curses
 * pad.
 * @return 0 under normal use, 1 when message is ready to be sent
 */
int handle_input();

/**
 * Uses the current cursor position to scroll the pad if required, moves the
 * cursor back into the right place, and then refreshes the pad.
 * @param cur_y The y position of the cursor
 * @param cur_y The x position of the cursor
 */
void update_pad(unsigned int cur_y, unsigned int cur_x);


#endif