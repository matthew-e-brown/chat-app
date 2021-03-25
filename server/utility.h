#ifndef __SERVER_FUNCTIONS__
#define __SERVER_FUNCTIONS__

#include "./constants.h"

/**
 * Get a pointer to a thread based on a username.
 * @param username The username to search for.
 * @return A pointer to that user's thread or NULL on not finding anything.
 */
Thread* get_thread_by_username(const char username[]);

#endif