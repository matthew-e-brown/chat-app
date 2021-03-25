#ifndef __CLIENT_INPUT__
#define __CLIENT_INPUT__

int handle_input();
void update_pad(unsigned int cur_y, unsigned int cur_x);
void get_cursor_pos(unsigned int buff_pos, unsigned int* y, unsigned int* x);

#endif