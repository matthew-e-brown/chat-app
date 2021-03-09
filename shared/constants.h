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


#define __GLOBAL_CONSTANTS__

// -------- Ease of use contants and macros --------

#define PR 0 // The side of pipes to read from
#define PW 1 // The side of pipes to write to

#define APP_VER 2
#define PORT 58289
#define MAX_EPOLL_EVENTS 10

#define NUM_ELEMS(x) (int)(sizeof(x) / sizeof((x)[0]))
#define MIN(x,y) (x <= y ? x : y)

// -------- Messaging Verbs --------

#define MSG_UNSET      0xff  // The seed/initial value for message types

// Messages from either party; internal; used by library functions only
#define ACK_PACKET     0x01  // Acknowledge all packets successful
#define ACK_PACK_ERR   0x0e  // Packet SHA hash error, please re-send
#define TRANSFER_END   0x0f  // Cancel the transfer partway through

// Messages from the client, from one client to another or to server
#define MSG_LOGIN      0x11  // Client is logging in with username
#define MSG_WHISPER    0x12  // Client is whispering from one client to another
#define MSG_BROADCAST  0x13  // Client is sending a message to all other users
#define MSG_COMMAND    0x1f  // Client is sending a command to the server

// Messages from the server directly
#define SRV_ANNOUNCE   0x21  // Server is announcing an update to all clients
#define SRV_RESPONSE   0x22  // Server is replying to an individual client
#define SRV_ERROR      0x23  // Server says, "something went wrong"; HTTP 500
#define USR_ERROR      0x24  // Server says, "user did sometihng wrong"; 400

// -------- Other Constants --------

#define USERNAME_MAX   16      // Maximum length for usernames