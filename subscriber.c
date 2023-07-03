// Copyright Raducanu George-Cristian Tema 2 PCOM

// Credit pentru schelet - Lab 7 de PCOM

/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP si mulplixare
 * client.c
 */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>

#include "common.h"
#include "helpers.h"
#include "structs.h"

#define MAX_PFDS 30

char my_id[100];
int my_port;
char my_ip[20];

uint32_t putere10(uint8_t put) {
  uint32_t rez = 1;
  while (put > 0) {
    put--;
    rez *= 10;
  }
  return rez;
}

void run_client(int sockfd) {
  char buf[MSG_MAXSIZE + 1];
  memset(buf, 0, MSG_MAXSIZE + 1);

  struct pollfd *pfds = calloc(MAX_PFDS, sizeof(struct pollfd));
  int nfds = 0;
  
  // introducere socket server si pt stdin
  pfds[nfds].fd = STDIN_FILENO;
  pfds[nfds].events = POLLIN;
  nfds++;
  
  pfds[nfds].fd = sockfd;
  pfds[nfds].events = POLLIN;
  nfds++;
 
while (1) {
    // se asteapta pana cand se primesc date pe cel putin unul din file descriptori
    poll(pfds, nfds, -1);
 
    if ((pfds[1].revents & POLLIN) != 0) {
        // se trateaza noua conexiune...
        // am primit pe socketul cu serverul
        struct udp_msg mesaj_udp;
        char buffy[1600] = {0};
        int rc1 = recv_all(sockfd, &buffy, sizeof(struct udp_msg));
        DIE(rc1 < 0, "Error receiving\n");
        memcpy(&mesaj_udp, buffy, rc1);
        if (mesaj_udp.type == 255) {
          printf ("Client %s already connected.\n", my_id);
          free(pfds);
          break;
        }

        // aici daca semnalul e de tip sa imi dau exit
        if (mesaj_udp.type == 254) {
          printf("Server stopped\n");
          free(pfds);
          break;
        }
        // daca nu am primit de kill mesaj nu ma omor

        printf("%s:%d - ", inet_ntoa(mesaj_udp.curr_ip_addr), mesaj_udp.curr_port);

        printf("%s - ", mesaj_udp.topic);
          uint8_t tip = 0;
          memcpy(&tip, &mesaj_udp.type, sizeof(uint8_t));
          if (tip == 0)
            printf("INT - ");
          if (tip == 1)
            printf("SHORT_REAL - ");
          if (tip == 2)
            printf("FLOAT - ");
          if (tip == 3)
            printf("STRING - ");

          if (tip == 0) {
            //printf("INT ");
            uint8_t semn = 0;
            memcpy(&semn, mesaj_udp.data, sizeof(uint8_t));
            uint32_t nr = 0;
            memcpy(&nr, mesaj_udp.data + 1, sizeof(uint32_t));
            nr = ntohl(nr);

            if (semn == 0) {
              printf("%u\n", nr);
            } else {
              printf("-%u\n", nr);
            }
          } else if (tip == 1) {
            uint16_t zecimal = 777;
            memcpy(&zecimal, mesaj_udp.data, sizeof(uint16_t));
            zecimal = ntohs(zecimal);

            printf("%u.%02u\n", zecimal / 100, zecimal % 100);
          } else if (tip == 2) {
            uint8_t semn = 0;
            memcpy(&semn, mesaj_udp.data, sizeof(uint8_t));
            uint32_t modul = 0;
            memcpy(&modul, mesaj_udp.data + 1, sizeof(uint32_t));
            modul = ntohl(modul);
            uint8_t pow10 = 0;
            memcpy(&pow10, mesaj_udp.data + sizeof(uint8_t) + sizeof(uint32_t), sizeof(uint8_t));
            uint32_t powy = putere10(pow10);
            float f = (float) modul;
            if (semn != 0) {
              f = -f;
            }
            f = 1.0 * f / (powy);
            printf("%.*f\n", pow10, f);
          } else {
            printf("%s\n", mesaj_udp.data);
          }
    }
    else if ((pfds[0].revents & POLLIN) != 0) {
        // citire date de la tatstatura
        memset(buf, 0, MSG_MAXSIZE + 1);
        fgets(buf, sizeof(buf), stdin);

        // acum tratam toate comenzile
        if (strncmp(buf, "exit", 4) == 0) {
          
          struct tcp_msg mesaj;
          mesaj.type = 'e';
          memset(mesaj.data, 0, sizeof(mesaj.data));
          strcpy(mesaj.data, my_id);
          send_all(sockfd, &mesaj, sizeof(mesaj));

          // mai trb si sa inchid toti fd deschisi
          for (int y = 0; y < nfds; ++y)
            close(pfds[y].fd);

          free(pfds);

          //printf("Client %s disconnected.\n", my_id);
          break;

        } else if (strncmp(buf, "subscribe", 9) == 0) {
          
          // in caz de subscribe trebuie tinut cont si de sf
          struct tcp_msg mesaj;
          memset(&mesaj, 0, sizeof(struct tcp_msg));
          mesaj.type = 's';

          strncpy(mesaj.data, buf + 10, strlen(buf + 10) - 3);
          mesaj.data[strlen(buf + 10) - 3] = '\0';

          mesaj.sf = buf[strlen(buf) - 2] - '0';
          //printf("Data buffer %s si apoi sf %d\n", mesaj.data, mesaj.sf);
          send_all(sockfd, &mesaj, sizeof(mesaj));
          printf("Subscribed to topic.\n");
        
        } else if (strncmp(buf, "unsubscribe", 11) == 0) {
          // comanda de unsubscribe
          struct tcp_msg mesaj;
          mesaj.type = 'u';
          memset(mesaj.data, 0, sizeof(mesaj.data));
          strcpy(mesaj.data, buf + 12);
          send_all(sockfd, &mesaj, sizeof(mesaj));
          printf("Unsubscribed from topic.\n");
        } else {
          // comanda invalida
          printf("Invalid command\n");
        }

    }
}
}

int main(int argc, char *argv[]) {
  // mare parte din main din lab

    // fara buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  int sockfd = -1;

  // asa apelez din linia de comanda  ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>
  strcpy(my_id, argv[1]);
  if (argc != 4) {
    printf("\n Usage: %s <id_client> <ip> <port>\n", argv[0]);
    return 1;
  }
  strcpy(my_ip, argv[2]);
  // Parsam port-ul ca un numar
  uint16_t port;
  int rc = sscanf(argv[3], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");
  my_port = port;
  // Obtinem un socket TCP pentru conectarea la server
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(sockfd < 0, "socket");

  // Completăm in serv_addr adresa serverului, familia de adrese si portul
  // pentru conectare
  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
  DIE(rc <= 0, "inet_pton");

  // Conectare la server
  rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "connect");

  // Se trimite ID-ul cu formatul vechi din lab, era deja facut
  // Aceasta structura se foloseste doar aici pt acest 
  // mic "handshake"
  struct chat_packet sent_packet;
  memset(&sent_packet, 0, sizeof(struct chat_packet));
  
  memcpy(sent_packet.message, argv[1], strlen(argv[1]));
  sent_packet.len = 1 + strlen(argv[1]);
  send_all(sockfd, &sent_packet, sizeof(sent_packet));

  run_client(sockfd);

  // Inchidem conexiunea si socketul creat
  close(sockfd);

  return 0;
}
