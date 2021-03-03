#define __GLOBAL_CONSTANTS__

// -------- Ease of use contants and macros --------

#define PR 0 // The side of pipes to read from
#define PW 1 // The side of pipes to write to

#define APP_VER 2
#define PORT 58289
#define EPOLL_MAX_EVENTS 10

#define NUM_ELEMS(x) (sizeof(x) / sizeof((x)[0]))

// -------- Messaging Verbs --------

// Messages from either party
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
#define SRV_ERROR      0x22  // Server says, "something went wrong"; HTTP 500

// -------- Other Constants --------

#define USERNAME_MAX   16      // Maximum length for usernames