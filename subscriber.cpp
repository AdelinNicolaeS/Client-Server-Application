#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
extern "C" {
#include "./helpers.h"
}
#include <algorithm>
#include <iostream>
#include <iterator>
#include <math.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// from the labs
void usage(char *file) {
  fprintf(stderr, "Usage: %s server_port\n", file);
  exit(0);
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0);
  if (argc != 4) {
    usage(argv[0]);
  }

  int sockfd, ret, fdmax;
  struct sockaddr_in serv_addr;
  char buffer[BUFLEN];
  fd_set read_fds, tmp_fds;

  FD_ZERO(&tmp_fds);
  FD_ZERO(&read_fds);

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(sockfd < 0, "socket");

  FD_SET(sockfd, &read_fds);
  fdmax = sockfd;
  FD_SET(0, &read_fds);

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(atoi(argv[3]));
  ret = inet_aton(argv[2], &serv_addr.sin_addr);
  DIE(ret == 0, "inet_aton");

  ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(ret < 0, "connect");
  FD_SET(STDIN_FILENO, &read_fds);
  FD_SET(sockfd, &read_fds);
  int flag_delay = 1;
  setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag_delay, sizeof(int));

  if (sockfd > STDIN_FILENO) {
    fdmax = sockfd;
  } else {
    fdmax = STDIN_FILENO;
  }
  ret = send(sockfd, argv[1], strlen(argv[1]), 0);
  DIE(ret < 0, "send");
  while (1) {
    tmp_fds = read_fds;
    ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
    DIE(ret < 0, "select");
    if (FD_ISSET(sockfd, &tmp_fds)) {
      // am primit mesajul deja format de la server
      // trebuie numai sa fie afisat
      uint32_t size;
      int rec_size = recv(sockfd, &size, sizeof(size), 0);
      DIE(rec_size < 0, "recv");

      int received = recv(sockfd, buffer, size, 0);
      DIE(received < 0, "recv");
      if (received == 0) {
        // comunicare intrerupta
        break;
      }
      printf("%s\n", buffer);
    } else if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
      // primim input de la tastatura, avem 3 (4) cazuri
      memset(buffer, 0, sizeof(buffer));
      fgets(buffer, BUFLEN - 1, stdin);
      if (buffer[strlen(buffer) - 1] == '\n') {
        buffer[strlen(buffer) - 1] = '\0';
      }
      if (strcmp(buffer, "exit") == 0) {
        break;
      }
      char buffer2[BUFLEN];
      strcpy(buffer2, buffer);
      if (strncmp(buffer, "subscribe", strlen("subscribe")) == 0) {
        char *word;
        word = strtok(buffer2, " "); // word = "subscribe"
        word = strtok(NULL, " ");    // word = topic?
        DIE(word == NULL, "no topic");
        word = strtok(NULL, " "); // word = sf?
        DIE(word == NULL, "no sf");
        DIE(strcmp(word, "0") != 0 && strcmp(word, "1") != 0, "sf wrong value");
        printf("Subscribed to topic.\n");
      } else if (strncmp(buffer, "unsubscribe", strlen("unsubscribe")) == 0) {
        char *word;
        word = strtok(buffer2, " "); // word = "unsubscribe"
        word = strtok(NULL, " ");    // word = topic?
        DIE(word == NULL, "no topic");
        printf("Unsubscribed from topic.\n");
      } else {
        // nici subscribe, nici unsubscribe, atunci error
        fprintf(stderr, "comanda nerecunoscuta la tcp client");
        exit(0);
      }
      // am ajuns aici, comanda este corecta, o trimitem la server
      uint32_t size_to_send = (uint32_t) (strlen(buffer) + 1);
      int ss = send(sockfd, &size_to_send, sizeof(size_to_send), 0);
      DIE(ss < 0, "send");
      int s = send(sockfd, buffer, strlen(buffer) + 1, 0);
      DIE(s < 0, "send");
    }
  }
  close(sockfd);
  return 0;
}