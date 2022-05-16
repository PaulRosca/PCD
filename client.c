#include "operations.h"
#include "socket_utils.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
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

void send_processing_request(int sfd, char *filepath, unsigned short operation,
                             char *argument) {
  write_bytes(sfd, client_id, 16);
  operation = htons(operation);
  write_bytes(sfd, &operation, 2);
  if (argument != NULL) {
    write_bytes(sfd, argument, 64);
  }
  send_file(filepath, sfd);
  free(filepath);
  if (argument != NULL) {
    free(argument);
  }
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
  while (1) {
    printf("Operations\nr.resize\nc.convert\nf.flip\nt.get tags\ne.exit\n");
    printf("Select an operation:");
    fflush(stdin);
    o = getchar(); // Get user option
    getchar();     // Get \n
    printf("\n");
    switch (o) {
    case 'r':
      send_processing_request(sockfd, get_image_path("Resize Image"), RESIZE,
                              get_operation_argument());
      receive_file(sockfd, "server_processed");
      break;
    case 'c':
      send_processing_request(sockfd, get_image_path("Convert Image"), CONVERT,
                              get_operation_argument());
      receive_file(sockfd, "server_processed");
      break;
    case 'f':
      send_processing_request(sockfd, get_image_path("Flip Image"), FLIP, NULL);
      receive_file(sockfd, "server_processed");
      break;
    case 't':
      send_processing_request(sockfd, get_image_path("Tag Image"), TAGS, NULL);
      char tags[128];
      read_bytes(sockfd, tags, 128);
      printf("Image tags are: %s", tags);
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

  close(sockfd);
  exit(EXIT_SUCCESS);
};
