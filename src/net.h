#ifndef _NET_H
#define _NET_H
#include <stdint.h>
#include <sys/types.h>

#include "error.h"

/*
 * Initializes the net class
 */
enum error net_init();

/*
 * Send and read data to and from the api
 * Returns the number of bytes sent/read or
 * a negative value which is the errno */
ssize_t net_send(const void *msg, size_t msg_len);
ssize_t net_read(void *out_data, size_t read_size);

/*
 * Frees the net class
 */
void net_free();

#endif /* _NET_H */
