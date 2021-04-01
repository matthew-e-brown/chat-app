/**
 * COIS-4310H: Chat App
 *
 * @name:         Chat App -- Message Encoding
 *
 * @author:       Matthew Brown, #0648289
 * @date:         March 29th to April 1st, 2021
 *
 * @purpose:      Holds code for "encoding" and "decoding" messages that are
 *                going to another client.
 *
 */


#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include <limits.h>

#include "./encoding.h"


// Random set of bytes used to obfuscate messages
const uint8_t key[32] = {
  0x14, 0x99, 0xd0, 0xc9, 0x31, 0x83, 0x52, 0x5b, 0x7d, 0x5a, 0x0a, 0xe5, 0x96,
  0x24, 0xf3, 0x23, 0x2e, 0x8b, 0x09, 0xad, 0x58, 0x13, 0x0c, 0x29, 0xcd, 0x8b,
  0x58, 0x81, 0xea, 0x2a, 0xd0, 0x46
};


/**
 * Rotate the bits of a number left
 * @param n The number to rotate
 * @param i The number of times to rotate-left
 * @returns The rotated number
 */
static inline uint32_t rotl32(uint32_t n, uint32_t i) {
  return ( n << i ) | ( n >> (-i & 31) );
}


/**
 * Rotate the bits of a number to the right
 * @param n The number to rotate
 * @param i The number of times to rotate-right
 * @returns The rotated number
 */
static inline uint32_t rotr32(uint32_t n, uint32_t i) {
  return ( n >> i ) | ( n << (-i & 31) );
}


ssize_t encode(const unsigned char *msg, size_t length, unsigned char **buff) {
  int i, o;
  uint8_t c;
  uint32_t p;
  size_t padding;

  // 1.  Check length of the message. Pad with 0x00 to a multiple of 4.
  // 2.  Allocate a buffer 2 bytes longer than the length + the padding.
  // 3.  In the first 2 bytes, store the original length of the message.
  //
  // --- For every four bytes of the message, where 'i' is the index into the
  // --- original string (grows by four) do the following:
  //
  // 1.  Put the 1st byte of the four into the highest 8 bits of a 32-bit
  //     integer, called 'p', the 2nd byte into the second highest, etc.
  // 2.  Select a character 'c' from the key string using the formula:
  //     > 'c' = key [ (i * msg_len) % key_len ]
  // 3.  Rotate 'p' left 'c' times
  // 4.  XOR 'p' with 'c' repeated four times (ie, if the 8-bit is '01101100',
  //     XOR with '01101100 01101100 01101100 01101100').
  // 5.  Take the lowest 8 bits of 'p' and put it into the i'th spot in the
  //     output buffer. Take the second lowest and put it into the i+1'th spot,
  //     and so on.

  if (length == 0) length = strlen((char*)(msg)) + 1;

  // If our message is too long to be sent in single chunk
  if (length > UINT16_MAX) return -1;

  padding = 0;
  while ((length + padding) % 4 != 0) padding += 1;

  (*buff) = calloc(2 + length + padding, 1);

  (*buff)[0] = ((uint16_t)(length)) & 0xff;
  (*buff)[1] = ((uint16_t)(length) >> 0x08) & 0xff;

  // 'o' adds the 2-byte gap at the start for the length
  for (i = 0, o = 2; i < (signed)(length + padding); i += 4, o += 4) {
    // Get current key indexer
    c = key[(i * length) % 32];

    // Get the 4-byte chunk
    p = (uint32_t)(msg[i + 0]) << 0x18 | (uint32_t)(msg[i + 1]) << 0x10 |
        (uint32_t)(msg[i + 2]) << 0x08 | (uint32_t)(msg[i + 3]) << 0x00 ;

    // Rotate left 'c' times
    p = rotl32(p, c);

    // XOR
    p ^= (uint32_t)(c) << 0x00 | (uint32_t)(c) << 0x08 |
         (uint32_t)(c) << 0x10 | (uint32_t)(c) << 0x18 ;

    // Save output
    (*buff)[o + 0] = (p >> 0x00) & 0xff;
    (*buff)[o + 1] = (p >> 0x08) & 0xff;
    (*buff)[o + 2] = (p >> 0x10) & 0xff;
    (*buff)[o + 3] = (p >> 0x18) & 0xff;
  }

  return 2 + length + padding;
}


size_t decode(const unsigned char *buff, unsigned char **msg) {
  int i, o;
  uint8_t c;
  uint32_t p;
  size_t length;

  // 1.  Read the first 2 bytes of the message to check how big of a buffer
  //     needs to be allocated (+1 for null terminator).
  //
  // --- For every four bytes of the remaining string (length determined by step
  // --- 1.), do the following:
  //
  // 1.  Put the 1st byte of the four into the lowest bytes of a 32-bit integer,
  //     'p', 2nd byte into the second-lowest, etc.
  // 2.  Get the c'th character of the key as an 8-bit integer from the formula:
  //     > 'c' = key [ (i * msg_len) % key_len ]
  // 3.  Create a 32-bit integer by repeating the 8-bits four times, and XOR 'p'
  //     with it.
  // 4.  Rotate 'p' right 'c' times.
  // 5.  Take the highest byte of 'p' and place it in the i'th byte of the
  //     output buffer. 2nd highest in the i+1'th byte, etc.

  // Get original length
  length = (buff[1] << 8) | (buff[0]);

  (*msg) = calloc(length, 1);

  // 'o' skips past the 2-byte length into the start for indexing into 'buffer'
  for (i = 0, o = 2; i < (signed)(length); i += 4, o += 4) {
    // Get current key indexer
    c = key[(i * length) % 32];

    // Get the inverted 4-byte chunk
    p = (uint32_t)(buff[o + 0]) << 0x00 | (uint32_t)(buff[o + 1]) << 0x08 |
        (uint32_t)(buff[o + 2]) << 0x10 | (uint32_t)(buff[o + 3]) << 0x18 ;

    // Undo XOR
    p ^= (uint32_t)(c) << 0x00 | (uint32_t)(c) << 0x08 |
         (uint32_t)(c) << 0x10 | (uint32_t)(c) << 0x18 ;

    // Rotate right 'c' times
    p = rotr32(p, c);

    // Save output
    (*msg)[i + 0] = (p >> 0x18) & 0xff;
    (*msg)[i + 1] = (p >> 0x10) & 0xff;
    (*msg)[i + 2] = (p >> 0x08) & 0xff;
    (*msg)[i + 3] = (p >> 0x00) & 0xff;
  }

  return length;
}