#include "operations.h"
#include "socket_utils.h"
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/un.h>
#include <unistd.h>

#define UNIX_SOCK_PATH "/tmp/pcd_unix_sock"

void print_order(struct orders *order) {
  if (order->operation == 102 || order->operation == 199) {
    printf("Order no: %llu, client_id: %s, operation: %d\n",
           order->order_number, order->client_id, order->operation);

  } else {
    printf("Order no: %llu, client_id: %s, operation: %d, argument: %s\n",
           order->order_number, order->client_id, order->operation,
           order->argument);
  }
}

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

  unsigned short op = 155;
  /* write_bytes(sockfd, &op, 2); */
  /* printf("Sent: %d\n", op); */
  char o = '\0';
  struct orders order;
  while (1) {
    printf("Operations\np.Get pending orders\nf.Get finished orders\nc.Cancel "
           "order\ne.exit\n");
    printf("Select an operation:");
    fflush(stdin);
    o = getchar(); // Get user option
    getchar();     // Get \n
    printf("\n");
    switch (o) {
    case 'p':
      op = htons(A_GET_PENDING);
      write_bytes(sockfd, &op, 2);
      printf("\nPending Orders\n");
      read_bytes(sockfd, &order, sizeof(struct orders));
      order.order_number = ntohl(order.order_number);
      order.operation = ntohs(order.operation);
      while (order.order_number != 0) {
        print_order(&order);
        bzero(&order, sizeof(order));
        read_bytes(sockfd, &order, sizeof(struct orders));
        order.order_number = ntohl(order.order_number);
        order.operation = ntohs(order.operation);
      };
      bzero(&order, sizeof(order));
      break;
    case 'f':
      op = htons(A_GET_FINISHED);
      write_bytes(sockfd, &op, 2);
      printf("\nFinished Orders\n");
      read_bytes(sockfd, &order, sizeof(struct orders));
      order.order_number = ntohl(order.order_number);
      order.operation = ntohs(order.operation);
      while (order.order_number != 0) {
        print_order(&order);
        bzero(&order, sizeof(order));
        read_bytes(sockfd, &order, sizeof(struct orders));
        order.order_number = ntohl(order.order_number);
        order.operation = ntohs(order.operation);
      };
      bzero(&order, sizeof(order));
      break;
    case 'c':
      op = htons(A_CANCEL);
      unsigned long long order_no;
      printf("Order number:");
      scanf("%llu", &order_no);
      getchar();
      order_no = htonl(order_no);
      write_bytes(sockfd, &op, 2);
      write_bytes(sockfd, &order_no, sizeof(order_no));
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
