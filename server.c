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

struct orders *front_pending, *back_pending, *front_finished = NULL,
                                             *back_finished;

pthread_mutex_t pending_queue, finished_queue;
pthread_cond_t c_var;
unsigned long long order_no = 1;

int is_empty(struct orders *front) { return front == NULL; };

void enqueue_order(struct orders *new_order, struct orders **front,
                   struct orders **back) {
  new_order->next = NULL;
  if (is_empty(*front)) {
    *front = new_order;
    *back = *front;
  } else {
    (*back)->next = new_order;
    *back = new_order;
  }
};

struct orders *dequeue(struct orders **front) {
  struct orders *order = NULL;
  if (!is_empty(*front)) {
    order = *front;
    *front = (*front)->next;
  }
  return order;
};

void cancel_order(struct orders **front, unsigned long long number) {
  struct orders *it = *front;
  if (is_empty(it)) {
    return;
  }
  struct orders *ord;
  if (it->order_number == number) {
    printf("Got here with number %llu\n", number);
    ord = it;
    *front = it->next;
    free(ord);
    return;
  }
  while (it->next) {
    if (it->next->order_number == number) {
      ord = it->next;
      it->next = it->next->next;
      free(ord);
      return;
    }
    it = it->next;
  }
};

void print_queue(struct orders *front) {
  while (front != NULL) {
    printf("Order no:%llu, client_id: %s, op: %d argument: %s\n",
           front->order_number, front->client_id, front->operation,
           front->argument);
    front = front->next;
  }
}

struct orders *receive_processing_request(int i) {
  struct orders *order = NULL;
  uuid_t client_id;
  bzero(client_id, 16);
  /* __off_t filesize = 0; */
  unsigned short op;
  char arg[64];
  /* char ext[6]; */
  if (read(i, &client_id, 1) <= 0) {
    return order;
  }
  order = malloc(sizeof(struct orders));
  order->cfd = i;
  read_bytes(i, &client_id[1], 15);
  printf("From client UUID: %s\n", client_id);
  read_bytes(i, &op, 2);
  op = ntohs(op);
  printf("From client Operation: %d\n", op);
  if (op != FLIP && op != TAGS) {
    read_bytes(i, &arg, 64);
    printf("From client argument: %s\n", arg);
  }
  pthread_mutex_lock(&pending_queue);
  order->order_number = order_no;
  order_no++;
  pthread_mutex_unlock(&pending_queue);
  char *filename = malloc(25 * sizeof(char));
  sprintf(filename, "%llu", order->order_number);
  char *fpth = receive_file(i, filename);
  strncpy(order->file_path, fpth, strlen(fpth));
  printf("File read: %s\n", fpth);
  memcpy(order->client_id, client_id, 16);
  order->operation = op;
  strncpy(order->argument, arg, 64);
  return order;
}

void send_list(int sfd, struct orders *front) {
  struct orders to_send;
  while (front != NULL) {
    memcpy(&to_send, front, sizeof(struct orders));
    to_send.operation = htons(to_send.operation);
    to_send.order_number = htonl(to_send.order_number);
    write_bytes(sfd, &to_send, sizeof(struct orders));
    front = front->next;
  }
  struct orders null_order;
  null_order.order_number = htonl(0);
  write_bytes(sfd, &null_order, sizeof(struct orders));
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
  unsigned long long ord_no;
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
      } else {
        read(admin_sock, ((void *)&op) + 1, 1);
        op = ntohs(op);
        switch (op) {
        case A_GET_PENDING:
          pthread_mutex_lock(&pending_queue);
          send_list(admin_sock, front_pending);
          pthread_mutex_unlock(&pending_queue);
          break;
        case A_GET_FINISHED:
          pthread_mutex_lock(&finished_queue);
          send_list(admin_sock, front_finished);
          pthread_mutex_unlock(&finished_queue);
          break;
        case A_CANCEL:
          read_bytes(admin_sock, &ord_no, sizeof(ord_no));
          ord_no = ntohl(ord_no);
          printf("Ord %llu\n", ord_no);
          pthread_mutex_lock(&finished_queue);
          cancel_order(&front_pending, ord_no);
          pthread_mutex_unlock(&finished_queue);
          break;
        default:
          break;
        }
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
          struct orders *new_order = receive_processing_request(i);
          if (new_order == NULL) {
            close(i);
            FD_CLR(i, &active_fd_set);
            printf("Closed a client connection\n");
            continue;
          } else {
            pthread_mutex_lock(&pending_queue);
            enqueue_order(new_order, &front_pending, &back_pending);
            pthread_cond_signal(&c_var);
            pthread_mutex_unlock(&pending_queue);
          }
        }
      }
  }
  return NULL;
};

void *web_func(void *arg) { return NULL; };

char *get_filename(char *filepath, char *extP) {
  char *processed_fp = malloc(50 * sizeof(char));
  char *it = filepath;
  int i = 0;
  while (it != (extP - 1)) {
    processed_fp[i++] = *it;
    it++;
  }
  processed_fp[i] = '\0';
  strcat(processed_fp, "_processed.");
  return processed_fp;
}

void *processing_func(void *arg) {
  while (1) {
    pthread_mutex_lock(&pending_queue);
    while (is_empty(front_pending)) {
      pthread_cond_wait(&c_var, &pending_queue);
    }
    printf("Processing pending queue\n");
    while (!is_empty(front_pending)) {
      struct orders *to_process = dequeue(&front_pending);
      pthread_mutex_unlock(&pending_queue);
      char cmd[256];
      char ext[6];
      char *processed_fp;
      char *extP;
      int i = 0;
      switch (to_process->operation) {
      case RESIZE:
        extP = get_filename_ext(to_process->file_path);
        processed_fp = get_filename(to_process->file_path, extP);
        strcat(processed_fp, extP);
        sprintf(cmd, "gm convert -resize %s %s %s", to_process->argument,
                to_process->file_path, processed_fp);
        printf("Running command %s\n", cmd);
        system(cmd);
        remove(to_process->file_path);
        send_file(processed_fp, to_process->cfd);
        remove(processed_fp);
        free(processed_fp);
        break;
      case CONVERT:
        extP = get_filename_ext(to_process->file_path);
        processed_fp = get_filename(to_process->file_path, extP);
        strcat(processed_fp, to_process->argument);
        sprintf(cmd, "gm convert %s %s", to_process->file_path, processed_fp);
        printf("Running command %s\n", cmd);
        system(cmd);
        remove(to_process->file_path);
        send_file(processed_fp, to_process->cfd);
        remove(processed_fp);
        free(processed_fp);
        break;
      case FLIP:
        extP = get_filename_ext(to_process->file_path);
        processed_fp = get_filename(to_process->file_path, extP);
        strcat(processed_fp, extP);
        sprintf(cmd, "gm convert -flip %s %s", to_process->file_path,
                processed_fp);
        printf("Running command %s\n", cmd);
        system(cmd);
        remove(to_process->file_path);
        send_file(processed_fp, to_process->cfd);
        remove(processed_fp);
        free(processed_fp);
        break;
      case TAGS:
        sprintf(cmd, "/usr/bin/python3 image-classifier.py %s",
                to_process->file_path);
        printf("Running command %s\n", cmd);

        FILE *fp;
        char output[128];

        /* Open the command for reading. */
        fp = popen(cmd, "r");
        if (fp == NULL) {
          printf("Failed to run command\n");
          exit(1);
        }

        /* Read the output a line at a time - output it. */
        fgets(output, sizeof(output), fp);
        write_bytes(to_process->cfd, output, 128);
        /* close */
        pclose(fp);

        remove(to_process->file_path);
      default:
        break;
      }
      pthread_mutex_lock(&finished_queue);
      enqueue_order(to_process, &front_finished, &back_finished);
      pthread_mutex_unlock(&finished_queue);
    }
  }

  return NULL;
};

int main(int argc, char **argv) {
  front_pending = NULL;
  back_pending = NULL;
  front_finished = NULL;
  back_finished = NULL;

  pthread_mutex_init(&pending_queue, NULL);
  pthread_mutex_init(&finished_queue, NULL);

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

  pthread_mutex_destroy(&pending_queue);
  exit(EXIT_SUCCESS);
};
