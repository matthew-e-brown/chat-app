#ifndef __SHARED_ENCODING__
#define __SHARED_ENCODING__

#include <stdlib.h>

/**
 * Encodes the given buffer using some secret sauce
 * @param msg The buffer to be encoded
 * @param length The size of the buffer to be encoded. If this parameter is
 * zero, the buffer is treated as a string and the strlen is used as the length
 * (plus one, to account for NUL terminator)
 * @param buff A pointer to a buffer to allocate and place the encded bytes into
 * @returns The length of the newly encoded buffer. -1 If the message was too
 * long to fit into the chunk
 */
ssize_t encode(const unsigned char *msg, size_t length, unsigned char **buff);


/**
 * Decodes a message by removing secret sauce from the given buffer.
 * @param buff The buffer to decode
 * @param msg A pointer to a buffer to allocate and place the original message
 * in
 * @returns The length of the decoded buffer
 */
size_t decode(const unsigned char *buff, unsigned char **msg);

#endif