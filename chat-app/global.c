#define __GLOBAL__

#define PR 0 // Read side of pipes
#define PW 1 // Write side of pipes

#define APP_VER 1
#define PORT 58289

#define MAX_EPOLL_EVENTS 10

#define NUM_ELEMS(x) (sizeof(x) / sizeof((x)[0]))

#define MSG_LOGIN 1
#define MSG_WHISPER 2
#define MSG_BROADCAST 3
#define MSG_COMMAND 4

#define MSG_ANNOUNCE 10
#define MSG_ACKNOWLEDGE 11
#define MSG_ACK_WITH_ERR 12

#define MAX_NAME_LEN 16
#define MAX_MSG_LEN 256

#include <stdlib.h>
#include <string.h>

struct header {
  unsigned short application_version;
  unsigned char message_type;
  unsigned short packet_count;
  unsigned short packet_index;
  char sender_name[MAX_NAME_LEN];
  char receiver_name[MAX_NAME_LEN];

  // keep extra space for future versions of the application
  char __padding__[
    128 - (sizeof(unsigned short) * 3) - (sizeof(unsigned char))
        - (sizeof(char) * MAX_NAME_LEN * 2)
  ];
};

typedef struct packet {
  struct header header;
  char body[MAX_MSG_LEN];
} Packet;


/**
 * Returns a new header with given parameters
 * @param message_type The defined constant for which type of message it is
 * @param sender_name The sender's name -- `0` for the server
 * @param receiver_name The recipient's name
 * @return The new header struct
 */
struct header create_header(
  unsigned char message_type,
  char *sender_name,
  char *receiver_name
) {
  struct header h;

  h.application_version = APP_VER;
  h.message_type = message_type;

  if (sender_name != NULL)
    strncpy(h.sender_name, sender_name, MAX_NAME_LEN);
  else
    memset(h.sender_name, 0, MAX_NAME_LEN);

  if (receiver_name != NULL)
    strncpy(h.receiver_name, receiver_name, MAX_NAME_LEN);
  else
    memset(h.receiver_name, 0, MAX_NAME_LEN);

  memset(h.__padding__, 0, sizeof h.__padding__);

  // user will have to fill packet index and count on their own
  return h;
}