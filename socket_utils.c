#include "socket_utils.h"
#include <unistd.h>

void read_bytes(int fd, void *buff, size_t size) {
  size_t bytes_left = size;
  int bytes_read = 0;
  while (bytes_left > 0) {
    bytes_read = read(fd, buff, bytes_left);
    bytes_left -= bytes_read;
    buff += bytes_read;
  }
};

void write_bytes(int fd, void *buff, size_t size) {
  size_t bytes_left = size;
  int bytes_sent = 0;
  while (bytes_left > 0) {
    bytes_sent = write(fd, buff, bytes_left);
    bytes_left -= bytes_sent;
    buff += bytes_sent;
  };
};
