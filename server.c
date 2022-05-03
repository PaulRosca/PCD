#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <uuid/uuid.h>

#define INET_PORT 5555

pthread_t admin_thread, client_thread, web_thread, processing_thread;

struct orders {
  unsigned long long order_number;
  uuid_t client_id;
  short operation;
  struct orders *next;
} * front_order, *back_order, *front_finished, *back_finished;

void write_bytes(int fd, void *buff, size_t size) {
  size_t bytes_left = size;
  size_t bytes_sent = 0;
  while (bytes_left > 0) {
    bytes_sent = write(fd, buff, bytes_left);
    bytes_left -= bytes_sent;
    buff += bytes_sent;
  };
};

void read_bytes(int fd, void *buff, size_t size) {
  size_t bytes_left = size;
  int bytes_read = 0;
  while (bytes_left > 0) {
    bytes_read = read(fd, buff, bytes_left);
    bytes_left -= bytes_read;
    buff += bytes_read;
  }
};

int recieve_file(int sfd, size_t filesize) {
  int fp = open("request.webp", O_WRONLY | O_CREAT);
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
    printf("READ: %s\n", buff);
    printf("BYTES_READ: %lu\n",bytes_read);
    write_bytes(fp, buff, bytes_read);
    bytes_left -= bytes_read;
    bzero(buff, 1024);
    total_read_bytes += bytes_read;
  }
  close(fp);
  return total_read_bytes;
};

void *admin_func(void *arg) { return NULL; };

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

  printf("Successfully created inet socket!\n");

  bzero(&serveraddr, sizeof(serveraddr));

  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons(INET_PORT);

  if ((bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) != 0) {
    perror("Error binding inet socket");
    exit(EXIT_FAILURE);
  }

  if (listen(sock, SOMAXCONN) < 0) {
    perror("listen");
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
          uuid_t client_id;
          unsigned long filesize = 0;
          read_bytes(i, client_id, 16);
          printf("From client UUID: %s\n", client_id);
          read_bytes(i, &filesize, 8);
          printf("From client FS: %lu\n", filesize);
          printf("File size read: %d\n", recieve_file(i, filesize));
          /* int size = read(i, buff, 4); */
          /* if (read(i, , 4) <= 0) { */
          /*   close(i); */
          /*   FD_CLR(i, &active_fd_set); */
          /* } */
        }
      }
  }
  return NULL;
};

void *web_func(void *arg) { return NULL; };

void *processing_func(void *arg) { return NULL; };

int is_empty(struct orders *front) { return front == NULL; };

void enqueue_order(uuid_t client_id, short operation) {
  struct orders *new_order = malloc(sizeof(struct orders));
  memcpy(new_order->client_id, client_id, 16);
  new_order->operation = operation;
  new_order->next = NULL;
  if (is_empty(front_order)) {
    new_order->order_number = 0;
    front_order = new_order;
    back_order = front_order;
  } else {
    new_order->order_number = back_order->order_number + 1;
    back_order->next = new_order;
    back_order = new_order;
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
    printf("Order no:%llu, client_id: %s, op: %d\n", front->order_number,
           front->client_id, front->operation);
    front = front->next;
  }
}

int main(int argc, char **argv) {
  front_order = NULL;
  back_order = NULL;
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
