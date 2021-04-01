/**
 * COIS-4310H: Chat App
 *
 * @name:         Chat App -- Global constants
 *
 * @author:       Matthew Brown, #0648289
 * @date:         February 1st to February 12th, 2021  
 *                March 1st to March 9th, 2021
 *
 * @purpose:      This file holds global constants used by both clietn and
 *                server. Some are macros, but most of them are simply numerical
 *                constants.
 *
 */

#ifndef __GLOBAL_CONSTANTS__
#define __GLOBAL_CONSTANTS__

// -------- Ease of use contants and macros --------

#define PR 0 // The side of pipes to read from
#define PW 1 // The side of pipes to write to

#define APP_VER 3
#define PORT 58289
#define MAX_EPOLL_EVENTS 10

#define NUM_ELEMS(x) (int)(sizeof(x) / sizeof((x)[0]))
#define MIN(x,y) (x <= y ? x : y)

// -------- Messaging Verbs --------

#define MSG_UNSET      ((unsigned short)(0xffff))  // The seed/initial value for message types

// Messages from either party; internal; used by library functions only
#define ACK_PACKET     ((unsigned short)(0x0001))  // Acknowledge a successful packet
#define ACK_PACK_ERR   ((unsigned short)(0x000e))  // Packet SHA hash error, please re-send
#define TRANSFER_END   ((unsigned short)(0x000f))  // Cancel the transfer partway through

// Messages from the client, from one client to another or to server
#define MSG_LOGIN      ((unsigned short)(0x1001))  // Client is logging in with username
#define MSG_WHISPER    ((unsigned short)(0x1002))  // Client is whispering from one client to another
#define MSG_BROADCAST  ((unsigned short)(0x1003))  // Client is sending a message to all other users
#define MSG_COMMAND    ((unsigned short)(0x100f))  // Client is sending a command to the server

#define MSG_ENC_WHISP  ((unsigned short)(0x1012))  // Encoded version of whisper
#define MSG_ENC_BROAD  ((unsigned short)(0x1013))  // Encoded version of broadcast

// Messages from the server directly
#define SRV_ANNOUNCE   ((unsigned short)(0x2001))  // Server is announcing an update to all clients
#define SRV_RESPONSE   ((unsigned short)(0x2002))  // Server is replying to an individual client
#define SRV_ERROR      ((unsigned short)(0x200e))  // Server says, "something went wrong"; HTTP 500
#define USR_ERROR      ((unsigned short)(0x200f))  // Server says, "user did something wrong"; 400

#define MASK_TYPE      ((unsigned short)(0xf000))  // Mask to check category
#define MSG_IS_ACK     ((unsigned short)(0x0000))  // The form after masking of a ACK_ type message
#define MSG_IS_MSG     ((unsigned short)(0x1000))  // The form after masking of a MSG_ type message
#define MSG_IS_SRV     ((unsigned short)(0x2000))  // The form after masking of a SRV_ type message

#define MASK_ENCODE    ((unsigned short)(0x00f0))  // Mask to check if MSG_ is encoded
#define MSG_IS_ENC     ((unsigned short)(0x0010))  // The form after masking to check if encoded

// -------- Other Constants --------

#define USERNAME_MAX   16                          // Maximum length for usernames

#endif