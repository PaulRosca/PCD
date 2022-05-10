#include "operations.h"
#include "socket_utils.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <uuid/uuid.h>

#define PORT 5555

uuid_t client_id;

char *get_filename_ext(char *filename) {
  char *dot = strrchr(filename, '.');
  if (!dot || dot == filename)
    return "";
  return dot + 1;
};

int send_file(char *filepath, int sfd) {
  int fp = open(filepath, O_RDONLY);
  if (fp < 0) {
    perror("Error opening file!");
    return 1;
  }
  __off_t size = lseek(fp, 0l, SEEK_END);
  printf("File size is %ld\n", size);
  write_bytes(sfd, &size, 8); // Send file size to server
  char ext[6];
  strcpy(ext, get_filename_ext(filepath));
  printf("Extension: %s\n", ext);
  write_bytes(sfd, ext, 6);
  lseek(fp, 0L, SEEK_SET);
  char data[1024] = {0};
  size_t total_bytes_sent = 0;
  __off_t bytes_left = size;
  __off_t bytes_sent = 0;
  while (bytes_left > 0) {
    bytes_sent = read(fp, data, 1024 > bytes_left ? bytes_left : 1024);
    write_bytes(sfd, data, bytes_sent); // Send file content to server
    /* printf("WRITE: %s\n", data); */
    /* printf("BYTES_WRITE: %ld\n", bytes_sent); */
    bzero(data, 1024);
    bytes_left -= bytes_sent;
    total_bytes_sent += bytes_sent;
  }
  printf("Done sending image!\n");
  printf("Bytes sent: %lu\n", total_bytes_sent);
  close(fp);
  return 0;
};

void send_processing_request(int sfd, char *filepath, unsigned short operation,
                             char argument[64]) {
  write_bytes(sfd, client_id, 16);
  write_bytes(sfd, &operation, 2);
  write_bytes(sfd, argument, 64);
  send_file(filepath, sfd);
  free(filepath);
}

char *get_image_path(const char *selected_option) {
  char *img_path = malloc(512 * sizeof(char));
  size_t len = 0;
  printf("%s\n", selected_option);
  printf("\nImage path:");
  ssize_t chars = getline(&img_path, &len, stdin);
  if ((img_path[chars - 1]) == '\n') {
    img_path[chars - 1] = '\0';
  }
  return img_path;
};

char *get_operation_argument() {
  char *arg = malloc(64 * sizeof(char));
  size_t len = 0;
  printf("Argument:");
  ssize_t chars = getline(&arg, &len, stdin);
  if ((arg[chars - 1]) == '\n') {
    arg[chars - 1] = '\0';
  }
  return arg;
};

int main(int argc, char **argv) {

  int sockfd;
  struct sockaddr_in servaddr;

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
  printf("\n\n");
  char o = '\0';
  char* argument;
  while (1) {
    printf("Operations\nr.resize\ne.exit\n");
    printf("Select an operation:");
    fflush(stdin);
    o = getchar(); // Get user option
    getchar();     // Get \n
    printf("\n");
    switch (o) {
    case 'r':
      send_processing_request(sockfd, get_image_path("Resize Image"), RESIZE,
                              get_operation_argument());
      break;
    case 'e':
      close(sockfd);
      exit(EXIT_SUCCESS);
      break;
    default:
      printf("Option \"%c\" is not a valid option!\n", o);
      break;
    };
    printf("\n");
  }

  /* write_bytes(sockfd, client_id, 16); */
  /* send_file("./image.webp", sockfd); */
  close(sockfd);
  exit(EXIT_SUCCESS);
};
