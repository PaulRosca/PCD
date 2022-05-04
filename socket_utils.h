#ifndef SOCKET_UTILS_H_
#define SOCKET_UTILS_H_

#include <sys/types.h>

void read_bytes(int fd, void *buff, size_t size);

void write_bytes(int fd, void *buff, size_t size);

#endif // SOCKET_UTILS_H_
