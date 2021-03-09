/**
 * COIS-4310H: Chat App
 *
 * @name:         Chat App -- Main client
 *
 * @author:       Matthew Brown, #0648289
 * @date:         February 1st to February 12th, 2021  
 *                March 1st to March 9th, 2021
 *
 * @purpose:      Connects to a server running on the port defined in
 *                `/shared/constants.h` and allows a user to send and receive
 *                broadcasts and whispers to and from other connected clients.
 *
 * @usage:        ./client.o name addr
 *
 * @parameters:   - name :: the username of the connecting client  
 *                - addr :: the remote address to connect to. Can be either a
 *                          hostname or an IP address.
 *
 * @example:      ./client.o matt localhost
 *                ./client.o jacques 192.168.2.82
 *
 * ===========================================================================
 *
 * Aside from this `main.c`, all other files in this directory have their
 * @purpose rule filled with a description of the files *themselves*, as opposed
 * to the whole program.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/epoll.h>

#include <curses.h>

#include "../shared/constants.h"
#include "../shared/messaging.c"
#include "../shared/utility.c"

#include "./constants.h"
#include "./input.c"
#include "./messages.c"

// -- Global Variables

WINDOW* chat_window;
WINDOW* text_window;

char current_message[MSG_BUFF];    // The current message being typed
char my_username[USERNAME_MAX];    // The user's username
unsigned int pos = 0;              // The current cursor position in the buffer

// -- Function Headers

static void setup_curses();
static void parse_args(int argc, char* argv[], in_addr_t* addr);
static void server_login(int* sock_fd, struct sockaddr* addr, socklen_t* size);


/**
 * Client's main function.
 * @param argc Count of arguments; from command line
 * @param argv Argument values; from command line
 * @return A status code; 0 on succcess
 */
int main(int argc, char* argv[]) {
  int rc;             // return code for various functions
  unsigned int y, x;  // used for positioning cursor in pad

  int server_sock;                 // socket FD for main server
  struct sockaddr_in server_addr;  // address of the main server
  socklen_t serv_a_size = sizeof(server_addr);

  int n, epoll_fd, num_events;
  struct epoll_event events[MAX_EPOLL_EVENTS];

  // >> Make sure there's null bytes in there
  memset(current_message, '\0', MSG_BUFF);

  // >> Get username and address
  parse_args(argc, argv, &server_addr.sin_addr.s_addr);
  server_addr.sin_port = htons(PORT);
  server_addr.sin_family = AF_INET;

  // >> Log into server
  server_login(&server_sock, (struct sockaddr*)&server_addr, &serv_a_size);

  // >> Setup epoll
  rc = setup_epoll(&epoll_fd, (int[]){ server_sock, STDIN_FILENO }, 2);

  if (rc != 0) {
    switch (rc) {
      case -1: perror("epoll_create1"); break;
      case 1: perror("epoll_add server_sock"); break;
      case 2: perror("epoll_add stdin"); break;
    }

    close(server_sock);
    exit(1);
  }

  // >> Finally, set up Curses library
  setup_curses();

  // >> Main event loop
  while (1) {
    num_events = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);

    if (num_events == -1) {
      endwin();
      close(server_sock);
      perror("epoll_wait");
      exit(1);
    }

    for (n = 0; n < num_events; n++) {

      if (events[n].data.fd == STDIN_FILENO) {

        if (handle_input()) {

          Message request = parse_buffer();

          // >> Hardcode checking for if the command is '/bye'
          if (
            request.type == MSG_COMMAND &&                // is a command that
            strstr(request.body, "bye") == request.body   // starts with bye
          ) goto exit;

          send_message(server_sock, request);
          display_own_message(request);

          // Extra check not to free the main message buffer because I don't
          // trust myself
          if (request.body != NULL && request.body != current_message)
            free(request.body);

          // >> Reset message
          memset(current_message, '\0', MSG_BUFF);
          pos = 0;

          // >> Redraw empty chat box
          get_cursor_pos(pos, &y, &x);

          werase(text_window);
          update_pad(y, x);
        }

      } else if (events[n].data.fd == server_sock) {

        Message response = recv_message(server_sock);

        if (response.type == MSG_UNSET) {
          // >> Socket closed
          endwin();
          printf("Lost connection to server.\n");
          close(server_sock);
          goto exit;
        }

        // >> Display and free the server's message
        display_message(response);
        if (response.body != NULL) free(response.body);

        // >> Reposition cursor in pad after drawing message
        get_cursor_pos(pos, &y, &x);
        update_pad(y, x);
      }

    }
  }

  exit:;

  // Can leave unfreed memory as is, since this is the very end of the program.
  // I have faith in the OS to take back everything it allocated the program.

  if (!isendwin()) endwin();
  return 0;
}


/**
 * Sets up curses library, windows, and pads. Draws borders for the two windows.
 * Extracted here to keep clutter out of main.
 */
static void setup_curses() {
  initscr();

  cbreak();
  noecho();
  nonl();

  clear();

  // >> Check if they can do colors, store result in global variable
  if (has_colors()) {
    start_color();

    init_pair(  CPAIR_BROADCAST,  COLOR_GREEN,   COLOR_BLACK  );
    init_pair(  CPAIR_WHISPER,    COLOR_CYAN,    COLOR_BLACK  );
    init_pair(  CPAIR_OWN_MSG,    COLOR_BLUE,    COLOR_BLACK  );
    init_pair(  CPAIR_NOTICE,     COLOR_YELLOW,  COLOR_BLACK  );
    init_pair(  CPAIR_ERROR,      COLOR_RED,     COLOR_BLACK  );
  }

  // >> Create the borders for the two windows, which are stationary and never
  //    need to be updated

  WINDOW* chat_border = newwin(LINES - 5, COLS, 0, 0);
  wborder(chat_border, '|', '|', '-', '-', '+', '+', '+', '+');

  WINDOW* text_border = newwin(5, COLS, LINES - 5, 0);
  wborder(text_border, '|', '|', '-', '-', '+', '+', '+', '+');
  mvwprintw(text_border, 1, 2, prompt_message);

  // >> Create the main chat window and the pad to type into. Also display
  //    welcome message and set properties

  chat_window = newwin(LINES - 7, COLS - 4, 1, 2);
  scrollok(chat_window, TRUE);
  wmove(chat_window, 0, 0);
  wprintw(chat_window, welcome_message);
  wprintw(chat_window, "\n<<------------------------>>\n\n");

  // Pad needs to be big enough to hold all possible characters, even if they're
  // all horizontal or all vertical
  text_window = newpad(MSG_BUFF, MSG_BUFF);
  keypad(text_window, TRUE);
  nodelay(text_border, TRUE);

  // >> Refresh all

  wrefresh(chat_border);
  wrefresh(chat_window);
  wrefresh(text_border);
  prefresh(
    text_window,
    0, 0,                              // (y, x) of pad to display
    LINES - 4, 2 + prompt_length + 1,  // (y, x) on term to start drawing
    LINES - 2, COLS - 3                // (y, x) on term to finish drawing
  );
}


/**
 * Reads arguments and gets the server address from the second argument and the
 * username in the first. Username is copied directly into buffer, address is
 * set by way of passed pointer.
 * @param argc Pass-through of argc from main
 * @param argv Pass-through of argv from main
 * @param addr Pointer to the address to set
 */
static void parse_args(int argc, char* argv[], in_addr_t* addr) {
  int i; // counter
  FILE* f; // which file to print to (stdout/err) when jumping to print usage

  // >> First thing's first, check if they needed help
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      // >> They need help, so print usage message and abort
      printf("ONCE YOU'RE INSIDE:\n%s\n\n", welcome_message);
      f = stdout;
      printf("STARTING THE APP: ");
      goto print_usage;
    }
  }

  if (argc != 3) {
    if (argc < 2) fprintf(stderr, "Missing username as argument.\n");
    if (argc < 3) fprintf(stderr, "Missing server address as argument.\n");
    if (argc > 3) fprintf(stderr, "Too many arguments given.\n");
    f = stderr;
    goto print_usage;
  }

  if (strlen(argv[1]) >= USERNAME_MAX) {
    fprintf(stderr, "Username is too long.\n");
    f = stderr;
    goto print_usage;
  } else {
    strncpy(my_username, argv[1], USERNAME_MAX);
  }

  // >> Attempt to read IP from arguments
  in_addr_t result = inet_addr(argv[2]);

  if (result != (in_addr_t)-1) {
    (*addr) = result;
  } else {

    // >> IP didn't work, try hostname
    struct hostent* hp = gethostbyname(argv[2]);

    if (hp == NULL) {
      fprintf(stderr, "Couldn't get IP address or hostname.\n");
      f = stderr;
      goto print_usage;
    } else {
      (*addr) = ((struct in_addr*)(hp->h_addr_list[0]))->s_addr;
    }

  }

  return;

print_usage:
  fprintf(f,
    "Usage:\n\n"
    " >> %s username host\n\n"
    "where 'username' is at most %i characters and 'host' is either an IP\n"
    "address or a hostname.\n",
    argv[0], USERNAME_MAX - 1
  );

  exit(2);
}


/**
 * Logs into the server.
 * @param sock_fd A pointer to the FD to put the socket on
 * @param addr A pointer to place the address in
 * @param size Used for bind; pointer to value in main
 */
static void server_login(int* sock_fd, struct sockaddr* addr, socklen_t* size) {

  // >> Create socket and establish connection

  if ((*sock_fd = socket(AF_INET, SOCK_STREAM, 0))  < 0) {
    perror("socket");
    exit(1);
  }

  if (connect(*sock_fd, addr, *size) < 0) {
    fprintf(stderr,
      "Could not connect to server.\n"
      "Is it running? Did you get the address right?\n");
    close(*sock_fd);
    exit(1);
  }

  Message request;

  request.type = MSG_LOGIN;
  request.size = 0;
  request.body = NULL;
  memcpy(request.sender_name, my_username, USERNAME_MAX);
  memset(request.receiver_name, 0, USERNAME_MAX);

  if (send_message(*sock_fd, request) != 0) {
    fprintf(stderr, "Login request failed.\n");
    close(*sock_fd);
    exit(1);
  }

  Message response = recv_message(*sock_fd);

  if (response.type != SRV_RESPONSE) {

    if (response.size > 0 && response.body != NULL) {
      fprintf(stderr, "%s error. Server said, \"%s\"\n",
        response.body, (response.type == USR_ERROR) ? "User" : "Server");
      free(response.body);
    } else {
      fprintf(stderr, "An unknown error occurred. Could not log in.\n");
    }

    close(*sock_fd);
    exit(1);
  }
}