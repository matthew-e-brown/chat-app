// -- Includes

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/epoll.h>

#include <sys/ioctl.h>
#include <ncurses.h>

// -- My Includes

#include "../global.c"
#include "./constants.h"

#include "./handlers.c"

// -- Function headers

in_addr_t parse_options(int argc, char* argv[]);

// -- Global variables

WINDOW *chat_window; // The window for the incoming messages
WINDOW *text_window; // The window for inputting characters

int server_sock;
struct sockaddr_in server_addr;
socklen_t serv_a_size = sizeof(server_addr);

char my_username[MAX_NAME_LEN];     // buffer of username passed on command line
char current_message[MAX_MSG_LEN];  // current message buffer
unsigned int msg_l = 0;             // current message length
unsigned int cur_x, cur_y;          // current cursor x,y positions


/**
 * The server's main function.
 * @returns an int; a return code. Zero on no error.
 */
int main(int argc, char* argv[]) {
  int return_code;
  int bytes_read;
  Packet message;

  // >> Get server address info from user arguments

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = parse_options(argc, argv);

  // >> Set up curses

  initscr();

  if (has_colors() == FALSE) {
    endwin();
    fprintf(stderr, "damn bro, your terminal doesn't do colours? rough.\n");
    exit(1);
  }

  cbreak();
  noecho();
  clear();

  start_color();
  init_pair(SERVER_PAIR, COLOR_YELLOW, COLOR_BLACK);
  init_pair(WHISPER_PAIR, COLOR_CYAN, COLOR_BLACK);
  init_pair(BROADCAST_PAIR, COLOR_GREEN, COLOR_BLACK);
  init_pair(OWN_PAIR, COLOR_BLUE, COLOR_BLACK);

  // >> Create the windows

  // The border around the main chat window. Never updated past this point

  WINDOW* chat_outline = newwin(LINES - 3, COLS, 0, 0);
  wborder(chat_outline, '|', '|', '-', '-', '+', '+', '+', '+');
  wrefresh(chat_outline);

  // The main chat window itself, sits inside the border created above

  chat_window = newwin(LINES - 5, COLS - 4, 1, 2);
  scrollok(chat_window, TRUE); // let the main window scroll
  wmove(chat_window, 0, 0); // reset position
  // >> print the welcome message in the chat window
  wprintw(chat_window, welcome_message);
  wprintw(chat_window, "\n<<------------------------>>\n");

  // The window at the bottom that the user types into

  text_window = newwin(3, COLS, LINES - 3, 0);
  keypad(text_window, TRUE); // read DEL + BACKSPACE
  wtimeout(text_window, 0); // non-blocking getch
  wborder(text_window, '|', '|', '-', '-', '+', '+', '+', '+');
  wmove(text_window, 1, 2); // reset position to just away from the border
  wprintw(text_window, prompt_message); // print prompt

  // >> Update both windows

  wrefresh(chat_window);
  wrefresh(text_window);

  // ============================================= END OF NCURSES WINDOW SETUP

  wattr_on(chat_window, COLOR_PAIR(SERVER_PAIR), NULL);

  // >> Connect to the server

  if (  (server_sock = socket(AF_INET, SOCK_STREAM, 0))  < 0) {
    endwin();
    perror("socket");
    exit(1);
  }

  if (connect(server_sock, (struct sockaddr*)&server_addr, serv_a_size) < 0) {
    endwin();
    fprintf(stderr, "Could not connect to server, is it running?\n");
    perror("connect");
    exit(1);
  }

  // >> Connection successful
  wprintw(chat_window, "Connection to server succesful!\n");
  wrefresh(chat_window);

  // >> Log into server

  message.header = create_header(MSG_LOGIN, my_username, 0);
  message.header.packet_count = 1;
  message.header.packet_index = 0;

  bytes_read = send(server_sock, &message, sizeof message, 0);

  if (bytes_read <= 0) {
    endwin();
    if (bytes_read == 0)
      fprintf(stderr, "Could not send to server???\n");
    else
      perror("send");
    exit(1);
  }

  // >> Get acknowledgement

  bytes_read = recv(server_sock, &message, sizeof message, 0);

  if (bytes_read <= 0) {
    endwin();
    if (bytes_read == 0)
      fprintf(stderr, "Server acknowledgement not received...\n");
    else
      perror("recv");
    exit(1);
  }

  wprintw(
    chat_window,
    "Successfully logged into server with username \"%s\"!\n",
    my_username
  );

  wattr_off(chat_window, COLOR_PAIR(SERVER_PAIR), NULL);

  wrefresh(chat_window);

  // ===================================== ONE-TIME CONNECTION SETUP COMPLETE
  // ----------------------------------->> START WHILE LOOP SETUP

  // >> Set up epoll
  struct epoll_event ev, events[MAX_EPOLL_EVENTS];
  int num_events, n;
  int epoll_fd = epoll_create1(0);

  if (epoll_fd == -1) {
    endwin();
    close(server_sock);
    perror("epoll create");
    exit(1);
  }

  ev.events = EPOLLIN;
  ev.data.fd = server_sock;

  return_code = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sock, &ev);

  if (return_code) {
    endwin();
    close(server_sock);
    perror("epoll add server_sock");
    exit(1);
  }

  ev.events = EPOLLIN;
  ev.data.fd = STDIN_FILENO; // also listen to stdin

  return_code = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);

  if (return_code) {
    endwin();
    close(server_sock);
    perror("epoll add stdin");
    exit(1);
  }

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
        handle_input();
      } else if (events[n].data.fd == server_sock) {
        Packet message;
        bytes_read = recv(server_sock, &message, sizeof message, 0);

        if (bytes_read <= 0 && bytes_read != sizeof message) {
          // seems to be reading zero
          // https://stackoverflow.com/questions/9747646/epoll-recv-return-value
          endwin();
          close(server_sock);
          fprintf(stderr, "recv read %i bytes? socket dead?\n", bytes_read);
          exit(1);
        } else {
          display_message(&message);
        }
      }
    }
  }

  endwin();
  return 0;
}


/**
 * Parses argc and argv to get the internet address to connect to
 * @param argc the count of arguments from main
 * @param argv the values of the arguments from main
 * @returns the internet address
 */
in_addr_t parse_options(int argc, char* argv[]) {
  if (argc != 3) {
    if (argc < 2) fprintf(stderr, "Missing username as argument.\n");
    if (argc < 3) fprintf(stderr, "Missing destination as argument.\n");
    if (argc > 3) fprintf(stderr, "Too many arguments given.\n");

    fprintf(
      stderr,
      "Usage:\n\n"
      " >> %s username host\n\n"
      "where 'username' is at most %i characters, and 'host' is either an IP\n"
      "address or a hostname.\n",
      argv[0], MAX_NAME_LEN
    );

    exit(2);
  }

  // >> copy username
  strncpy(my_username, argv[1], MAX_NAME_LEN);

  // >> get IP address from arguments
  in_addr_t result = inet_addr(argv[2]);
  if (result != (in_addr_t)-1) return result;

  // >> IP didn't work, try getting the hostname instead
  struct hostent *hp = gethostbyname(argv[2]);
  if (hp == NULL) {
    fprintf(stderr, "Couldn't get IP address or host-name\n");
    exit(2);
  }

  return ((struct in_addr*)(hp->h_addr_list[0]))->s_addr;
}