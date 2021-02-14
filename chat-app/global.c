/**
 * COIS-4310H: Chat App
 *
 * @name:         global.c
 *
 * @author:       Matthew Brown, #0648289
 * @date:         February 1st to February 12th, 2021
 *
 * @purpose:      This file holds many global constants for both the server and
 *                the client to refer to.
 *
 */


#define __GLOBAL__

#define PR 0 // Read side of pipes
#define PW 1 // Write side of pipes

#define APP_VER 1
#define PORT 58289

#define MAX_EPOLL_EVENTS 10

#define NUM_ELEMS(x) (sizeof(x) / sizeof((x)[0]))

// The "verbs" that each Packet can be describing

#define MSG_LOGIN 1           // the client wishes to login
#define MSG_WHISPER 2         // the client is whispering
#define MSG_BROADCAST 3       // the client is broadcasting
#define MSG_COMMAND 4         // the client is sending a command

#define MSG_ANNOUNCE 10       // the server is sending an announcement
#define MSG_ACKNOWLEDGE 11    // one party is acknowledging another's message
#define MSG_ACK_WITH_ERR 12   // the server is telling the client of an error

#define MAX_NAME_LEN 16       // the maximum length that a username can be
#define MAX_MSG_LEN 256       // the maximum length that a message can be

#include <stdlib.h>
#include <string.h>

// Structs for the packet datatypes

struct header {
  unsigned short application_version; // Which version of Chat App is this?
  unsigned short message_type;        // What verb is this message for?
  unsigned short packet_count;        // How many packets are carrying it?
  unsigned short packet_index;        // Which of those packets is this?
  char sender_name[MAX_NAME_LEN];     // Who is it from?
  char receiver_name[MAX_NAME_LEN];   // Who is it going to?

  // keep extra space for future versions of the application
  char __padding__[
    128 - (sizeof(unsigned short) * 4) - (sizeof(char) * MAX_NAME_LEN * 2)
  ];
};

typedef struct packet {
  struct header header;       // Packet metadata
  char body[MAX_MSG_LEN];     // The raw bytes of the message
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

  if (sender_name != NULL) {
    strncpy(h.sender_name, sender_name, MAX_NAME_LEN);
  } else {
    memset(h.sender_name, 0, MAX_NAME_LEN);
  }

  if (receiver_name != NULL) {
    strncpy(h.receiver_name, receiver_name, MAX_NAME_LEN);
  } else {
    memset(h.receiver_name, 0, MAX_NAME_LEN);
  }

  memset(h.__padding__, 0, sizeof h.__padding__);

  // user will have to fill packet index and count on their own
  return h;
}