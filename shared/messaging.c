#define __MESSAGING__

#ifndef __GLOBAL_CONSTANTS__
#include "constants.h"
#endif


#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>


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
    char sender_name[USERNAME_MAX];
    char receiver_name[USERNAME_MAX];
    char checksum[SHA_DIGEST_LENGTH];

    char __padding__[
      128 - (sizeof(unsigned short) * 4) - (sizeof(char) * USERNAME_MAX * 2)
          - (sizeof(char) * SHA_DIGEST_LENGTH)
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


/**
 * Sends an empty packet with just the message_type field set; for
 * acknowledgements during send/receive.
 * @param socket The socket to send to
 * @param message_type The acknowledgement type to be made
 */
static void ping(int socket, unsigned short message_type) {
  Packet packet;

  packet.header.app_ver = APP_VER;
  packet.header.packet_count = 1;
  packet.header.packet_index = 0;

  memset(packet.header.sender_name, 0, USERNAME_MAX);
  memset(packet.header.receiver_name, 0, USERNAME_MAX);
  memset(packet.header.checksum, 0, SHA_DIGEST_LENGTH);

  packet.header.message_type = message_type;

  /* size_t b_sent = */ send(socket, &packet, sizeof(Packet), 0);
}


/**
 * Sends a message. Message is broken into chunks and sent one by one.
 * @param socket The socket file descriptor to send the message to
 * @param message The raw data to send inside the packets
 * @return A return code; 0 on success, anything else on error
 */
int send_message(int socket, Message message) {
  Packet packet;

  // How many chunks this packet will take
  unsigned short packet_count = message.size / PACKET_DATASIZE + 1;

  packet.header.app_ver = APP_VER;
  packet.header.message_type = message.type;
  packet.header.packet_count = packet_count;

  strncpy(packet.header.sender_name, message.sender_name, USERNAME_MAX);
  strncpy(packet.header.receiver_name, message.receiver_name, USERNAME_MAX);

  size_t remaining_size = message.size;
  unsigned short p_indx = 0;

  while (remaining_size > 0) {
    Packet response;
    packet.header.packet_index = p_indx;

    // Determine which slice of the message buffer to send
    size_t offset = PACKET_DATASIZE * p_indx;
    size_t amount = min(PACKET_DATASIZE, remaining_size);

    // Copy buffer chunk to packet and compute SHA1 hash
    memcpy(packet.data, message.body + offset, amount);
    SHA1(packet.data, PACKET_DATASIZE, packet.header.checksum);

send_packet:
    size_t b_sent = send(socket, &packet, sizeof(Packet), 0);
    if (b_sent == -1) {
      ping(socket, TRANSFER_END);
      return errno;
    }

    size_t b_recv = recv(socket, &response, sizeof(Packet), 0);
    if (b_recv == -1) {
      ping(socket, TRANSFER_END);
      return errno;
    }

    if (response.header.message_type == ACK_PACK_ERR) goto send_packet;
    else if (response.header.message_type != ACK_PACKET) return -1;

    p_indx += 1;
    remaining_size -= amount;
  }

}


/**
 * Receives a message. Message is read in packet-by-packet.
 * @param socket The socket to read from
 * @return A pointer Message struct containing the sent message and metadata
 */
Message recv_message(int socket) {
  Packet packet;
  Message output;
  char sha_buff[SHA_DIGEST_LENGTH];

  output.body = NULL; // seed value

  do {
recv_packet: // retry receipt after acknowledging failure

    // Receive the message
    size_t b_recv = recv(socket, &packet, sizeof(Packet), 0);
    if (b_recv == -1) {
      ping(socket, ACK_PACK_ERR);
      goto recv_packet;
    }

    // If the transfer was cancelled unexpectedly, return a blank message
    if (packet.header.message_type == TRANSFER_END) {
      if (output.body != NULL) free(output.body);

      output.size = 0;
      memset(output.sender_name, 0, USERNAME_MAX);
      memset(output.receiver_name, 0, USERNAME_MAX);

      output.type = TRANSFER_END;
      return output;
    }

    // Verify SHA1 hash
    SHA1(packet.data, PACKET_DATASIZE, sha_buff);
    if (strncmp(packet.header.checksum, sha_buff, SHA_DIGEST_LENGTH) != 0) {
      ping(socket, ACK_PACK_ERR);
      goto recv_packet;
    }

    // Create the required fields for the output message if they're not set yet
    if (output.body == NULL) {
      output.type = packet.header.message_type;
      output.body = calloc(packet.header.packet_count, PACKET_DATASIZE);

      strncpy(output.receiver_name, packet.header.receiver_name, USERNAME_MAX);
      strncpy(output.sender_name, packet.header.sender_name, USERNAME_MAX);
    }

    size_t offset = packet.header.packet_index * PACKET_DATASIZE;
    memcpy(output.body + offset, packet.data, PACKET_DATASIZE);

    ping(socket, ACK_PACKET);

  } while (packet.header.packet_index + 1 < packet.header.packet_count);

  return output;
}