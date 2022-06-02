#ifndef SOCKET_UTILS_H_
#define SOCKET_UTILS_H_

#include <sys/types.h>
#include <uuid/uuid.h>

struct orders {
  unsigned long long order_number;
  uuid_t client_id;
  unsigned short operation;
  char argument[64];
  char file_path[30];
  struct orders *next;
  int cfd;
};

void read_bytes(int fd, void *buff, size_t size);

void write_bytes(int fd, void *buff, size_t size);

char *receive_file(int sfd, char *filename);

int send_file(char *filepath, int sfd);

char *get_filename_ext(const char *filename, char delimiter);

#endif // SOCKET_UTILS_H_
