#ifndef __GLOBAL_MESSAGING__
#define __GLOBAL_MESSAGING__

#include <openssl/sha.h>


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


/**
 * Sends a message. Message is broken into chunks and sent one by one.
 * @param socket The socket file descriptor to send the message to
 * @param message The raw data to send inside the packets
 * @return A return code; 0 on success, anything else on error
 */
int send_message(int socket, Message message);


/**
 * Receives a message. Message is read in packet-by-packet.
 * @param socket The socket to read from
 * @return A Message struct containing the sent message and metadata; will be
 * empty on error
 */
Message recv_message(int socket);

#endif