#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/un.h>
#include <unistd.h>
#include "operations.h"
#include "socket_utils.h"

#define UNIX_SOCK_PATH "/tmp/pcd_unix_sock"

int main(int argc, char **argv) {
  int sockfd;
  struct sockaddr_un servaddr;

  // socket create and verification
  sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("Error creating socket");
    exit(EXIT_FAILURE);
  }

  bzero(&servaddr, sizeof(servaddr));

  servaddr.sun_family = AF_UNIX;
  strcpy(servaddr.sun_path, UNIX_SOCK_PATH);

  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
    perror("Error connecting to server");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  printf("connected to the server..\n");

  unsigned short op=155;
  /* write_bytes(sockfd, &op, 2); */
  /* printf("Sent: %d\n", op); */
  char o = '\0';
  while (1) {
    printf("Operations\np.Get pending orders\ne.exit\n");
    printf("Select an operation:");
    fflush(stdin);
    o = getchar(); // Get user option
    getchar();     // Get \n
    printf("\n");
    switch (o) {
    case 'p':
      op = A_GET_PENDING;
      write_bytes(sockfd, &op, 2);
      printf("Successfully sent %d to server\n",op);
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
  exit(EXIT_SUCCESS);
}
