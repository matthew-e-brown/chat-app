#ifndef __SERVER_THREAD__
#define __SERVER_THREAD__

/**
 * Handles ongoing communication between a client and the server
 * @param arg A void pointer to the thread struct this thread is responsible for
 */
void* client_thread(void* arg);

#endif