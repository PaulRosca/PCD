#include <arpa/inet.h>
#include <bits/types.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <uuid/uuid.h>

#define PORT 5555

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

int send_file(char *filepath, int sfd) {
  int fp = open(filepath, O_RDONLY);
  if (fp < 0) {
    printf("Error opening file!\n");
    return 1;
  }
  __off_t size = lseek(fp, 0l, SEEK_END);
  printf("File size is %ld\n", size);
  write_bytes(sfd, &size, 8); // Send file size
  lseek(fp, 0L, SEEK_SET);
  char data[1024] = {0};
  size_t total_bytes_sent = 0;
  __off_t bytes_left = size;
  __off_t bytes_sent = 0;
  while (bytes_left > 0) {
    bytes_sent = read(fp, data, 1024 > bytes_left ? bytes_left : 1024);
    write_bytes(sfd, data, bytes_sent); // Send file content
    printf("WRITE: %s\n", data);
    printf("BYTES_WRITE: %ld\n", bytes_sent);
    bzero(data, 1024);
    bytes_left -= bytes_sent;
    total_bytes_sent += bytes_sent;
  }
  printf("Done sending image!\n");
  printf("Bytes sent: %lu\n", total_bytes_sent);
  close(fp);
  return 0;
};

int main(int argc, char **argv) {

  int sockfd, connfd;
  struct sockaddr_in servaddr;
  uuid_t client_id;

  // socket create and verification
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("Error creating socket");
    exit(EXIT_FAILURE);
  }

  printf("Socket successfully created..\n");
  bzero(&servaddr, sizeof(servaddr));

  // assign IP, PORT
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  servaddr.sin_port = htons(PORT);

  // connect the client socket to server socket
  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
    perror("Error connecting to server");
    exit(EXIT_FAILURE);
  }
  printf("connected to the server..\n");

  unsigned char *uuid_read = client_id;
  read_bytes(sockfd, uuid_read, 16);
  printf("Client UUID: %s\n", client_id);
  write_bytes(sockfd, client_id, 16);
  send_file("./image.webp", sockfd);
  close(sockfd);
  exit(EXIT_SUCCESS);
};
