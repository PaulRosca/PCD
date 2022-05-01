#include <pthread.h>
#include <uuid/uuid.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pthread_t admin_thread, client_thread, web_thread, processing_thread;

struct orders {
  unsigned long long order_number;
  uuid_t client_id;
  short operation;
  struct orders *next;
} * front_order, *back_order, *front_finished, *back_finished;

void *admin_func(void *arg) { return NULL; };

void *client_func(void *arg) { return NULL; };

void *web_func(void *arg) { return NULL; };

void *processing_func(void *arg) { return NULL; };

int is_empty(struct orders *front) { return front == NULL; };


void enqueue_order(uuid_t client_id, short operation) {
    struct orders* new_order = malloc(sizeof(struct orders));
    memcpy(new_order->client_id, client_id, 16);
    new_order->operation = operation;
    new_order->next = NULL;
    if(is_empty(front_order)) {
        new_order->order_number = 0;
        front_order = new_order;
        back_order = front_order;
    }
    else {
        new_order->order_number = back_order->order_number + 1;
        back_order->next = new_order;
        back_order = new_order;
    }
};

void add_to_finished(struct orders *order) {
    order->next = NULL;
    if(is_empty(front_finished)) {
        front_finished = order;
        back_finished = front_finished;
    }
    else {
        back_finished->next = order;
        back_finished = order;
    }
}

struct orders* dequeue(struct orders **front) {
    struct orders* order = NULL;
    if(!is_empty(*front)) {
       order = *front;
       *front = (*front)->next;
    }
    return order;
};

void print_queue(struct orders* front) {
    while(front!=NULL) {
        printf("Order no:%llu, client_id: %s, op: %d\n", front->order_number, front->client_id, front->operation);
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
    perror("Error creating admin thread!");
    exit(EXIT_FAILURE);
  }

  if (pthread_create(&web_thread, NULL, web_func, NULL) != 0) {
    perror("Error creating admin thread!");
    exit(EXIT_FAILURE);
  }

  if (pthread_create(&processing_thread, NULL, processing_func, NULL) != 0) {
    perror("Error creating admin thread!");
    exit(EXIT_FAILURE);
  }

  pthread_join(admin_thread, NULL);
  pthread_join(client_thread, NULL);
  pthread_join(web_thread, NULL);
  pthread_join(processing_thread, NULL);
  exit(EXIT_SUCCESS);
};
