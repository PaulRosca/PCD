#include "operations.h"
#include "socket_utils.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <uuid/uuid.h>

#define INET_PORT 5555
#define UNIX_SOCK_PATH "/tmp/pcd_unix_sock"

pthread_t admin_thread, client_thread, web_thread, processing_thread;

struct orders {
  unsigned long long order_number;
  uuid_t client_id;
  unsigned short operation;
  char argument[64];
  struct orders *next;
} * front_pending, *back_pending, *front_finished, *back_finished;

pthread_mutex_t pending_queue;

int is_empty(struct orders *front) { return front == NULL; };

void enqueue_order(struct orders *new_order) {
  new_order->next = NULL;
  if (is_empty(front_pending)) {
    new_order->order_number = 0;
    front_pending = new_order;
    back_pending = front_pending;
  } else {
    new_order->order_number = back_pending->order_number + 1;
    back_pending->next = new_order;
    back_pending = new_order;
  }
};

void add_to_finished(struct orders *order) {
  order->next = NULL;
  if (is_empty(front_finished)) {
    front_finished = order;
    back_finished = front_finished;
  } else {
    back_finished->next = order;
    back_finished = order;
  }
}

struct orders *dequeue(struct orders **front) {
  struct orders *order = NULL;
  if (!is_empty(*front)) {
    order = *front;
    *front = (*front)->next;
  }
  return order;
};

void print_queue(struct orders *front) {
  while (front != NULL) {
    printf("Order no:%llu, client_id: %s, op: %d argument: %s\n",
           front->order_number, front->client_id, front->operation,
           front->argument);
    front = front->next;
  }
}

int recieve_file(int sfd, size_t filesize) {
  int fp = open("request.webp", O_WRONLY | O_CREAT, 0666);
  if (fp < 0) {
    printf("Error opening file!\n");
    return -1;
  }
  size_t bytes_left = filesize;
  size_t bytes_read = 0;
  size_t total_read_bytes = 0;
  char buff[1024];
  while (bytes_left > 0) {
    bytes_read = read(sfd, buff, 1024 > bytes_left ? bytes_left : 1024);
    /* printf("READ: %s\n", buff); */
    /* printf("BYTES_READ: %lu\n",bytes_read); */
    write_bytes(fp, buff, bytes_read);
    bytes_left -= bytes_read;
    bzero(buff, 1024);
    total_read_bytes += bytes_read;
  }
  close(fp);
  return total_read_bytes;
};

struct orders *recieve_processing_request(int i) {
  struct orders *order = NULL;
  uuid_t client_id;
  bzero(client_id, 16);
  unsigned long filesize = 0;
  unsigned short op;
  char arg[64];
  char ext[6];
  if (read(i, &client_id, 1) <= 0) {
    return order;
  }
  read_bytes(i, &client_id[1], 15);
  printf("From client UUID: %s\n", client_id);
  read_bytes(i, &op, 2);
  printf("From client Operation: %d\n", op);
  read_bytes(i, &arg, 64);
  printf("From client argument: %s\n", arg);
  read_bytes(i, &filesize, 8);
  printf("From client FS: %lu\n", filesize);
  read_bytes(i, ext, 6);
  printf("From client FE: %s\n", ext);
  printf("File size read: %d\n", recieve_file(i, filesize));
  order = malloc(sizeof(struct orders));
  memcpy(order->client_id, client_id, 16);
  order->operation = op;
  strncpy(order->argument, arg, 64);
  return order;
}

void *admin_func(void *arg) {
  int sock, admin_sock;
  struct sockaddr_un serveraddr, clientname;
  socklen_t size = sizeof(serveraddr);

  sock = socket(AF_UNIX, SOCK_STREAM, 0);

  if (sock == -1) {
    perror("Error creating unix socket");
    exit(EXIT_FAILURE);
  }

  printf("Successfully created unix socket!\n");

  bzero(&serveraddr, sizeof(serveraddr));

  serveraddr.sun_family = AF_UNIX;
  strcpy(serveraddr.sun_path, UNIX_SOCK_PATH);

  unlink(UNIX_SOCK_PATH);

  if ((bind(sock, (struct sockaddr *)&serveraddr, size)) == -1) {
    perror("Error binding unix socket");
    close(sock);
    exit(EXIT_FAILURE);
  }

  if (listen(sock, 0) < 0) {
    perror("Error listening on UNIX socket");
    close(sock);
    exit(EXIT_FAILURE);
  }

  int connected = 0;

  unsigned short op;
  while (1) {

    admin_sock = accept(sock, (struct sockaddr *)&clientname, &size);
    if (admin_sock < 0) {
      perror("Error accepting admin");
      close(sock);
      exit(EXIT_FAILURE);
    }

    printf("Server: connect from admin\n");
    connected = 1;

    while (connected) {
      if ((read(admin_sock, &op, 1)) <= 0) {
        close(admin_sock);
        printf("Closed admin connection\n");
        connected = 0;
        /* break; */
      } else {
        read(admin_sock, ((void *)&op) + 1, 1);
        printf("Read operation: %d\n", op);
      }
    }
  }

  return NULL;
};

void *client_func(void *arg) {
  int sock;
  socklen_t size;
  fd_set active_fd_set, read_fd_set;
  int i;
  struct sockaddr_in serveraddr, clientname;

  /* Create the socket and set it up to accept connections. */
  sock = socket(AF_INET, SOCK_STREAM, 0);

  if (sock == -1) {
    perror("Error creating inet socket");
    exit(EXIT_FAILURE);
  }

  int reuse = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse,
                 sizeof(reuse)) < 0) {

    perror("setsockopt(SO_REUSEADDR) failed");
  }

#ifdef SO_REUSEPORT
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse,
                 sizeof(reuse)) < 0) {
    perror("setsockopt(SO_REUSEPORT) failed");
  }
#endif

  printf("Successfully created inet socket!\n");

  bzero(&serveraddr, sizeof(serveraddr));

  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons(INET_PORT);

  if ((bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) != 0) {
    perror("Error binding inet socket");
    close(sock);
    exit(EXIT_FAILURE);
  }

  if (listen(sock, SOMAXCONN) < 0) {
    perror("Error listening on INET socket");
    close(sock);
    exit(EXIT_FAILURE);
  }

  /* Initialize the set of active sockets. */
  FD_ZERO(&active_fd_set);
  FD_SET(sock, &active_fd_set);

  while (1) {

    /* Block until input arrives on one or more active sockets. */
    read_fd_set = active_fd_set;
    if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
      perror("select");
      close(sock);
      exit(EXIT_FAILURE);
    }

    /* Service all the sockets with input pending. */
    for (i = 0; i < FD_SETSIZE; ++i)
      if (FD_ISSET(i, &read_fd_set)) {
        if (i == sock) {
          /* Connection request on original socket. */
          int new;
          uuid_t new_uuid;
          size = sizeof(clientname);
          new = accept(sock, (struct sockaddr *)&clientname, &size);
          if (new < 0) {
            perror("accept");
            close(sock);
            exit(EXIT_FAILURE);
          }
          fprintf(stderr, "Server: connect from host %s, port %hd.\n",
                  inet_ntoa(clientname.sin_addr), ntohs(clientname.sin_port));
          FD_SET(new, &active_fd_set);
          uuid_generate(new_uuid);
          printf("Generated uuid: %s\n", new_uuid);
          unsigned char *send_buff = new_uuid;
          write_bytes(new, send_buff, 16);
        } else {
          /* Data arriving on an already-connected socket. */
          struct orders *new_order = recieve_processing_request(i);
          if (new_order == NULL) {
            close(i);
            FD_CLR(i, &active_fd_set);
            printf("Closed a client connection\n");
            continue;
          } else {
            pthread_mutex_lock(&pending_queue);
            enqueue_order(new_order);
            print_queue(front_pending);
            pthread_mutex_unlock(&pending_queue);
          }
        }
      }
  }
  return NULL;
};

void *web_func(void *arg) { return NULL; };

void *processing_func(void *arg) { return NULL; };

int main(int argc, char **argv) {
  front_pending = NULL;
  back_pending = NULL;
  front_finished = NULL;
  back_finished = NULL;

  if (pthread_create(&admin_thread, NULL, admin_func, NULL) != 0) {
    perror("Error creating admin thread!");
    exit(EXIT_FAILURE);
  }

  if (pthread_create(&client_thread, NULL, client_func, NULL) != 0) {
    perror("Error creating client thread!");
    exit(EXIT_FAILURE);
  }

  if (pthread_create(&web_thread, NULL, web_func, NULL) != 0) {
    perror("Error creating web thread!");
    exit(EXIT_FAILURE);
  }

  if (pthread_create(&processing_thread, NULL, processing_func, NULL) != 0) {
    perror("Error creating processing thread!");
    exit(EXIT_FAILURE);
  }

  pthread_join(admin_thread, NULL);
  pthread_join(client_thread, NULL);
  pthread_join(web_thread, NULL);
  pthread_join(processing_thread, NULL);
  exit(EXIT_SUCCESS);
};
