#ifndef __GLOBAL_MESSAGING__
#define __GLOBAL_MESSAGING__

#include <openssl/sha.h>

#define PACKET_DATASIZE 256  // The most data a single packet can carry


/**
 * Packet definition defined by the RFC. Used for messaging between servers.
 */
typedef struct packet {
  struct packet_header {
    unsigned short app_ver;
    unsigned short message_type;
    unsigned short packet_count;
    unsigned short packet_index;
    size_t total_length;
    char sender_name[USERNAME_MAX];
    char receiver_name[USERNAME_MAX];
    unsigned char checksum[SHA_DIGEST_LENGTH];

    char __padding__[
      128 - (sizeof(unsigned short) * 4) - (sizeof(char) * USERNAME_MAX * 2)
          - (sizeof(char) * SHA_DIGEST_LENGTH) - (sizeof(size_t))
    ];
  } header;
  char data[PACKET_DATASIZE];
} Packet;


/**
 * Data type for messages. **Not** defined by the RFC. Used internally by client
 * and server before/after sending/receiving by way of the Packet type.
 */
typedef struct message {
  unsigned short type;
  char sender_name[USERNAME_MAX];
  char receiver_name[USERNAME_MAX];
  size_t size;
  char* body;
} Message;


int send_message(int socket, Message message);
Message recv_message(int socket);

#endif