#include "socket_utils.h"
#include <bits/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

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

char *recieve_file(int sfd, char *filename) {
  // Recieve file from sfd socket, creating it with name filename
  char ext[6];
  __off_t filesize = 0;
  read_bytes(sfd, &filesize, 8);
  read_bytes(sfd, ext, 6);
  char *file_path = malloc(30 * sizeof(char));
  sprintf(file_path, "%s.%s", filename, ext);
  int fp = open(file_path, O_WRONLY | O_CREAT, 0666);
  if (fp < 0) {
    printf("Error opening file!\n");
    return NULL;
  }
  size_t bytes_left = filesize;
  size_t bytes_read = 0;
  size_t total_read_bytes = 0;
  char buff[1024];
  while (bytes_left > 0) {
    bytes_read = read(sfd, buff, 1024 > bytes_left ? bytes_left : 1024);
    write_bytes(fp, buff, bytes_read);
    bytes_left -= bytes_read;
    bzero(buff, 1024);
    total_read_bytes += bytes_read;
  }
  close(fp);
  return file_path;
};

int send_file(char *filepath, int sfd) {
  // Send file from filepath over sfd socket
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
    bzero(data, 1024);
    bytes_left -= bytes_sent;
    total_bytes_sent += bytes_sent;
  }
  printf("Done sending image!\n");
  printf("Bytes sent: %lu\n", total_bytes_sent);
  close(fp);
  return 0;
};

char *get_filename_ext(char *filename) {
  char *dot = strrchr(filename, '.');
  if (!dot || dot == filename)
    return "";
  return dot + 1;
};
