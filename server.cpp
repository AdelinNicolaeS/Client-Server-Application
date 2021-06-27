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

using namespace std;

struct updated_subscription {
  char topic[50];
  vector<pair<uint32_t, string>> contents;
  int sf;
};

// from the labs
void usage(char *file) {
  fprintf(stderr, "Usage: %s server_port\n", file);
  exit(0);
}

void concatenate_strings(char buf_to_send[1600], char topic[51],
                         char name_of_data[12], char number[BUFLEN],
                         struct sockaddr_in cli_addr) {
  sprintf(buf_to_send, "%s:%hu", inet_ntoa(cli_addr.sin_addr),
          htons(cli_addr.sin_port));
  strcat(buf_to_send, " - ");
  strcat(buf_to_send, topic);
  strcat(buf_to_send, " - ");
  strcat(buf_to_send, name_of_data);
  strcat(buf_to_send, " - ");
  strcat(buf_to_send, number);
}

void build_message(int data_type, char input[1552], char content[1500],
                   char name_of_data[12], char number[BUFLEN]) {
  if (data_type == 0) { // int
    uint32_t *converted_content = (uint32_t *)(input + 52);
    if ((uint8_t) * (input + 51) == 1) {
      sprintf(number, "-%u", ntohl(*converted_content));
    } else {
      sprintf(number, "%u", ntohl(*converted_content));
    }
    strcpy(name_of_data, "INT");
  } else if (data_type == 1) { // short real
    uint16_t *converted_content = (uint16_t *)(input + 51);
    double real_content = (double)(ntohs(*converted_content)) / 100;
    sprintf(number, "%.2f", real_content);
    strcpy(name_of_data, "SHORT_REAL");
  } else if (data_type == 2) { // float
    uint32_t *converted_content = (uint32_t *)(input + 52);
    uint8_t *power = (uint8_t *)(input + 56);
    float real_content = (ntohl(*converted_content)) / pow(10, (*power));
    if ((uint8_t) * (input + 51) == 1) {
      sprintf(number, "-%f", real_content);
    } else {
      sprintf(number, "%f", real_content);
    }
    strcpy(name_of_data, "FLOAT");
  } else { // string
    strncpy(number, content, strlen(content));
    strcpy(name_of_data, "STRING");
  }
}

void from_udp_to_tcp(
    struct sockaddr_in &cli_addr, char input[1552], int i,
    unordered_set<int> &all_clients, unordered_set<string> &online_clients,
    unordered_map<string, vector<struct updated_subscription>> &client_topics,
    unordered_map<int, string> &fd_id) {
  char buf_to_send[1600];
  memset(buf_to_send, 0, sizeof(buf_to_send));

  socklen_t socklen = sizeof(cli_addr);
  memset(input, 0, sizeof(char) * 1552);
  int received =
      recvfrom(i, input, 1552, 0, (struct sockaddr *)&cli_addr, &socklen);
  DIE(received < 0, "recvfrom");
  char topic[51], content[1500];
  int data_type;
  memset(topic, 0, sizeof(topic));
  memset(content, 0, sizeof(content));

  strncpy(topic, input, 50);
  data_type = (int8_t)input[50];
  strncpy(content, input + 51, 1500);

  char number[BUFLEN];
  memset(number, 0, sizeof(number));

  char name_of_data[12];
  memset(name_of_data, 0, sizeof(name_of_data));
  build_message(data_type, input, content, name_of_data, number);
  concatenate_strings(buf_to_send, topic, name_of_data, number, cli_addr);

  for (auto client_id : all_clients) {
    string client = fd_id.at(client_id);
    for (auto &parser : client_topics.at(client)) {
      if (strcmp(parser.topic, topic) == 0) {
        // clientul e abonat la acest topic
        if (online_clients.find(client) !=
            online_clients.end()) { // client conectat
          uint32_t dim = (uint32_t)(strlen(buf_to_send) + 1);
          int ss = send(client_id, &dim, sizeof(dim), 0);
          DIE(ss < 0, "send");
          int s = send(client_id, buf_to_send, strlen(buf_to_send) + 1, 0);
          DIE(s < 0, "send");
        } else {
          // client deconectat, verific daca fac store-and-forward
          if (parser.sf == 1) {
            // fac store-forward
            pair<int, string> new_content;
            new_content.first = (uint32_t)(strlen(buf_to_send) + 1);
            new_content.second = (string)buf_to_send;
            parser.contents.push_back(new_content);
          }
        }
      }
    }
  }
}

void from_tcp_to_server(
    char input[1552], int i, fd_set &read_fds,
    unordered_set<string> &online_clients,
    unordered_map<string, vector<struct updated_subscription>> &client_topics,
    unordered_map<int, string> &fd_id) {
  memset(input, 0, BUFLEN);
  uint32_t size_c;
  int recv_size = recv(i, &size_c, sizeof(size_c), 0);
  DIE(recv_size < 0, "recv");
  int received = recv(i, input, size_c, 0);
  DIE(received < 0, "recv");
  if (received == 0) {
    // clientul se inchide, il eliminam din structurile auxiliare
    string id_name = fd_id.at(i);
    printf("Client %s disconnected.\n", id_name.c_str());
    online_clients.erase(id_name); // nu mai este conectat
    close(i);
    FD_CLR(i, &read_fds);
  } else {
    // am primit (un)subscribe, actionam corespunzator
    string id_name = fd_id.at(i);
    if (strncmp(input, "subscribe", strlen("subscribe")) == 0) {
      char *word = strtok(input, " ");
      word = strtok(NULL, " ");
      // word = topicul cerut
      struct updated_subscription new_subcriber;
      new_subcriber.contents.clear();
      strcpy(new_subcriber.topic, word);
      word = strtok(NULL, " "); // word = sf
      if (strcmp(word, "0") == 0) {
        new_subcriber.sf = 0;
      } else if (strcmp(word, "1") == 0) {
        new_subcriber.sf = 1;
      }
      client_topics.at(id_name).push_back(new_subcriber);
    } else if (strncmp(input, "unsubscribe", strlen("unsubscribe")) == 0) {
      char *word = strtok(input, " ");
      word = strtok(NULL, " ");
      // word = numele topic ului
      int ok = 0;
      int pos = -1;
      for (unsigned int i = 0; i < client_topics.at(id_name).size(); i++) {
        if (strcmp(client_topics.at(id_name).at(i).topic, word) == 0) {
          ok = 1;
          pos = i;
        }
      }
      if (ok == 0) {
        fprintf(stderr, "No subcription for this topic\n");
      } else {
        client_topics.at(id_name).erase(client_topics.at(id_name).begin() +
                                        pos);
        // stergem topicul aflat la pozitia pos din vectorul de topicuri
      }
    }
  }
}

void connection_request(
    struct sockaddr_in &cli_addr, char input[1552], int &fdmax, int &sockfd,
    fd_set &read_fds, unordered_set<string> &online_clients,
    unordered_map<string, vector<struct updated_subscription>> &client_topics,
    unordered_set<int> &all_clients, unordered_map<int, string> &fd_id) {
  socklen_t socklen = sizeof(cli_addr);

  int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &socklen);
  DIE(newsockfd < 0, "accept");
  memset(input, 0, BUFLEN);
  int received = recv(newsockfd, input, BUFLEN, 0);
  // input = id-ul
  DIE(received < 0, "recv");
  if (online_clients.find(input) != online_clients.end()) {
    // avem deja un tcp cu acelasi id - nu se poate asa ceva
    printf("Client %s already connected.\n", input);
    close(newsockfd);
    return;
  }
  int flag_delay = 1;
  setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, &flag_delay, sizeof(int));
  printf("New client %s connected from %s:%d.\n", input,
         inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

  FD_SET(newsockfd, &read_fds);
  if (newsockfd > fdmax) {
    fdmax = newsockfd;
  }

  bool first_time = (all_clients.find(newsockfd) == all_clients.end());
  all_clients.insert(newsockfd);    // nu conteaza daca era deja,
                                    // unordered_set nu pastreaza duplicate
  online_clients.insert(input);     // client online nou
  fd_id.insert({newsockfd, input}); // un nou client alaturi de id ul sau

  if (first_time) {
    // clientul nu e in map, facem o pereche noua
    vector<struct updated_subscription> new_topics; // gol
    client_topics.insert({input, new_topics});
  } else {
    // fusese deja conectat o data, trimitem tot ce era on hold
    for (auto tpc : client_topics.at(input)) {
      if (tpc.sf == 1) { // posibil redundant
        for (auto content : tpc.contents) {
          uint32_t size = content.first;
          int send_size = send(newsockfd, &size, sizeof(size), 0);
          DIE(send_size < 0, "send");
          int bytes = send(newsockfd, content.second.c_str(),
                           content.second.size() + 1, 0);
          DIE(bytes < 0, "send");
        }
      }
    }
    for (auto &tpc : client_topics.at(input)) {
      tpc.contents.clear();
    }
  }
}

void initialize_sockaddr_in(struct sockaddr_in &my_addr, int portno) {
  memset((char *)&my_addr, 0, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(portno);
  my_addr.sin_addr.s_addr = INADDR_ANY;
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0);
  if (argc < 2) {
    usage(argv[0]);
  }
  int sockfd, portno, udp_socket, i, ret, fdmax;
  char input[1552];
  struct sockaddr_in serv_addr, cli_addr;
  unordered_set<int> all_clients;
  unordered_set<string> online_clients; // retine id-urile
  unordered_map<string, vector<struct updated_subscription>> client_topics;
  unordered_map<int, string> fd_id;
  fd_set read_fds, tmp_fds;

  FD_ZERO(&read_fds);
  FD_ZERO(&tmp_fds);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(sockfd < 0, "sockfd");
  udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
  DIE(udp_socket < 0, "udp_socket");
  portno = atoi(argv[1]);
  DIE(portno == 0, "atoi");
  int flag_delay = 1;
  setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag_delay, sizeof(int));
  initialize_sockaddr_in(serv_addr, portno);
  initialize_sockaddr_in(cli_addr, portno);
  ret = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
  DIE(ret < 0, "bind sockfd");
  ret = bind(udp_socket, (struct sockaddr *)&cli_addr, sizeof(struct sockaddr));
  DIE(ret < 0, "bind udp_socket");
  ret = listen(sockfd, MAX_CLIENTS);
  DIE(ret < 0, "listen");
  FD_SET(STDIN_FILENO, &read_fds);
  FD_SET(sockfd, &read_fds);
  FD_SET(udp_socket, &read_fds);
  if (sockfd > udp_socket) {
    fdmax = sockfd;
  } else {
    fdmax = udp_socket;
  }
  int stop = 0;
  while (!stop) {
    tmp_fds = read_fds;
    ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
    DIE(ret < 0, "select while");
    for (i = 0; i <= fdmax; ++i) {
      if (FD_ISSET(i, &tmp_fds)) {
        if (i == STDIN_FILENO) {
          scanf("%s", input);
          if (strcmp(input, "exit") == 0) {
            stop = 1;
            break;
          }
        } else if (i == udp_socket) {
          from_udp_to_tcp(cli_addr, input, i, all_clients, online_clients,
                          client_topics, fd_id);

        } else if (i == sockfd) {
          // cerere de conexiune pe socketul cu listen, cel inactiv
          connection_request(cli_addr, input, fdmax, sockfd, read_fds,
                             online_clients, client_topics, all_clients, fd_id);
        } else {
          // mesaj primit de la un client tcp
          // vor fi primite de catre server
          from_tcp_to_server(input, i, read_fds, online_clients, client_topics,
                             fd_id);
        }
      }
    }
  }
  close(sockfd);
  return 0;
}
