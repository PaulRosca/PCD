#ifndef SOCKET_UTILS_H_
#define SOCKET_UTILS_H_

#include <sys/types.h>
#include <uuid/uuid.h>

struct orders {
  unsigned long long order_number;
  uuid_t client_id;
  unsigned short operation;
  char argument[64];
  struct orders *next;
};

void read_bytes(int fd, void *buff, size_t size);

void write_bytes(int fd, void *buff, size_t size);

#endif // SOCKET_UTILS_H_
