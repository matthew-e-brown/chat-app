/**
 * COIS-4310H: Chat App
 *
 * @name:         Chat App -- Global messaging functions
 *
 * @author:       Matthew Brown, #0648289
 * @date:         March 1st to March 9th, 2021
 *
 * @purpose:      This file holds functions used by the client and server to
 *                send messages back and forth. They act as wrappers to the
 *                built-in `send` and `recv` commands to comply with the RFC.
 *
 *                This implementation uses both a `message` struct and the
 *                `packet` struct. The `message` struct is used internally and
 *                the `packet` struct is only used within this file for
 *                `send_message` and `recv_message`.
 *
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "./constants.h"
#include "./messaging.h"


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
  packet.header.total_length = 0;

  memset(packet.header.sender_name, 0, USERNAME_MAX);
  memset(packet.header.receiver_name, 0, USERNAME_MAX);
  memset(packet.header.checksum, 0, SHA_DIGEST_LENGTH);

  packet.header.message_type = message_type;

  send(socket, &packet, sizeof(Packet), 0);
}


/**
 * Sends a message. Message is broken into chunks and sent one by one.
 * @param socket The socket file descriptor to send the message to
 * @param message The raw data to send inside the packets
 * @return A return code; 0 on success, anything else on error
 */
int send_message(int socket, Message message) {
  Packet packet;

#ifdef __DEBUG__
// Part of the assignment. See comments below at send_packet:; for details.

  char old[PACKET_DATASIZE];  // Save unmodified copy of data
  unsigned char reset = 0;    // 0/1 for if the corrupt packet was on purpose
#endif

  // How many chunks this packet will take?
  unsigned short packet_count = message.size / PACKET_DATASIZE + 1;

  packet.header.app_ver = APP_VER;
  packet.header.message_type = message.type;
  packet.header.packet_count = packet_count;
  packet.header.total_length = message.size;

  strncpy(packet.header.sender_name, message.sender_name, USERNAME_MAX);
  strncpy(packet.header.receiver_name, message.receiver_name, USERNAME_MAX);

  size_t remaining_size = message.size;
  unsigned short p_indx = 0;

  // Use do-while in case message size is zero (meaning it contains metadata
  // only). Like MSG_LOGIN, the important data is in sender_name
  do {
    Packet response;
    packet.header.packet_index = p_indx;

    // >> Determine which slice of the message buffer to send
    size_t offset = PACKET_DATASIZE * p_indx;
    size_t amount = MIN(PACKET_DATASIZE, remaining_size);

    // >> Copy buffer chunk to packet and compute SHA1 hash
    memcpy(packet.data, message.body + offset, amount);
    SHA1((unsigned char*)packet.data, PACKET_DATASIZE, packet.header.checksum);

send_packet:;

#ifdef __DEBUG__
// Part of the assignment. In order to know if we are successfully recovering
// from packet error, we force a corrupted packet. This needs to be AFTER the
// send_packet label, otherwise when the retry is attempted the error will still
// be present

    // 8% chance seems good
    if (rand() % 100 > 92 && amount > 0) {
      // Save old copy of data
      memcpy(old, packet.data, PACKET_DATASIZE);
      reset = 1;

      // >> Corrupt one byte of the packet with a random ascii from 32-126
      int index = rand() % (int)(amount - 1);
      packet.data[index] = (char)(rand() % (127 - 32) + 32);

      printf("__DEBUG__ :: Corrupted index %i of packet data with char '%c'!\n",
        index, packet.data[index]);
    }
#endif

    ssize_t b_sent = send(socket, &packet, sizeof(Packet), 0);
    if (b_sent == -1) {
      ping(socket, TRANSFER_END);
      return errno;
    }

    ssize_t b_recv = recv(socket, &response, sizeof(Packet), 0);
    if (b_recv == -1) {
      ping(socket, TRANSFER_END);
      return errno;
    }

    // >> Error check
    if (response.header.message_type == ACK_PACK_ERR) {

#ifdef __DEBUG__
      // If this was an intentional packet error, copy back from just_in_case
      // before jumping back to retry
      if (amount > 0 && reset == 1) {
        memcpy(packet.data, old, PACKET_DATASIZE);
        memset(old, 0, PACKET_DATASIZE);
        reset = 0;
      }

      fprintf(stderr, "[INTERNAL] Received ACK_PACK_ERR! Re-sending...\n");
#endif

      goto send_packet;
    } else if (response.header.message_type != ACK_PACKET) {
      ping(socket, TRANSFER_END);
      return -1;
    }

    p_indx += 1;
    remaining_size -= amount;

  } while (remaining_size > 0);

  return 0;
}


/**
 * Receives a message. Message is read in packet-by-packet.
 * @param socket The socket to read from
 * @return A Message struct containing the sent message and metadata; will be
 * empty on error
 */
Message recv_message(int socket) {
  Packet packet;
  Message output;
  char last_pack = 0;
  unsigned char sha_buff[SHA_DIGEST_LENGTH];

  // >> Seed/unset values
  output.size = 0;
  output.type = MSG_UNSET;
  output.body = NULL;

  do {
recv_packet:; // To retry receipt after acknowledging failure

    // >> Receive the packet
    ssize_t b_recv = recv(socket, &packet, sizeof(Packet), 0);
    if (b_recv == -1) {
      ping(socket, ACK_PACK_ERR);
      goto recv_packet;
    } else if (b_recv == 0) {
      return output; // return earlier
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

    // >> Check bool for if this is the last packet
    last_pack = !(packet.header.packet_index + 1 < packet.header.packet_count);

    // >> Verify SHA1 hash
    SHA1((unsigned char*)packet.data, PACKET_DATASIZE, sha_buff);
    if (
      strncmp(
        (char*)packet.header.checksum, (char*)sha_buff, SHA_DIGEST_LENGTH
      ) != 0
    ) {
      ping(socket, ACK_PACK_ERR);
      goto recv_packet;
    }

    // >> Create the required fields for the output message if not set yet
    if (output.type == MSG_UNSET) {
      output.type = packet.header.message_type;
      output.size = packet.header.total_length;

      if (output.size > 0) output.body = calloc(output.size, 1);

      strncpy(output.receiver_name, packet.header.receiver_name, USERNAME_MAX);
      strncpy(output.sender_name, packet.header.sender_name, USERNAME_MAX);
    }

    // >> If there is body-text, copy it over into the buffer
    if (output.size > 0) {
      size_t offset = packet.header.packet_index * PACKET_DATASIZE;
      size_t amount = !last_pack
        ? PACKET_DATASIZE
        : packet.header.total_length % PACKET_DATASIZE;

      memcpy(output.body + offset, packet.data, amount);
    }

    ping(socket, ACK_PACKET);

  } while (!last_pack);

  return output;
}