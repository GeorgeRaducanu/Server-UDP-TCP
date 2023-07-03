// Copyright Raducanu George-Cristian Tema 2 PCOM


// Credit pentru schelet - Lab 7 de PCOM
/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP
 * Echo Server
 * server.c
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include "./common.h"
#include "./helpers.h"
#include "structs.h"

#include <netinet/tcp.h>

#define MAX_CONNECTIONS 80

// variabile globale ce vor fi folosite cand primesc de la udp
// ca sa pun in structura mea si port si ip corespunzatoare udp ului
struct sockaddr_in serv_addr, udp_addr;
socklen_t socket_len = sizeof(struct sockaddr_in);

void run_chat_multi_server(int listenfd, int udp_sock) {

  // Aici vectorii mei de structuri alocati dinamic ca sa nu imi crape stiva
  // intrucat ocupa suficienta memorie
  struct pollfd *poll_fds = calloc(MAX_CONNECTIONS, sizeof(struct pollfd));
  struct id_topics *id_map_topics = calloc(MAX_CONNECTIONS, sizeof(struct id_topics));
  struct topic_ids *topic_map_id = calloc(MAX_CONNECTIONS, sizeof(struct topic_ids));
  struct info_unsent_id *id_unsent_msgs = calloc(MAX_CONNECTIONS, sizeof(struct info_unsent_id));

  int total_number_ids = 1;
  int num_topics = 1;
  int num_clients = 1;
  int rc;

  // Setam socket-ul listenfd pentru ascultare
  rc = listen(listenfd, MAX_CONNECTIONS);
  DIE(rc < 0, "listen");

  // se adauga noul file descriptor (socketul pe care se asculta conexiuni) in
  // multimea read_fds
  poll_fds[0].fd = listenfd;
  poll_fds[0].events = POLLIN;
  num_clients++;

  poll_fds[1].fd = udp_sock;
  poll_fds[1].events = POLLIN;
  num_clients++;

  // adaug alt socket pt stdin
  poll_fds[2].fd = STDIN_FILENO;
  poll_fds[2].events = POLLIN;
  num_clients++;

  int loop = 1;
  while (loop) {

    rc = poll(poll_fds, num_clients, -1);
    DIE(rc < 0, "poll");

    for (int i = 0; i < num_clients; i++) {
      if (poll_fds[i].revents & POLLIN) {
        if (poll_fds[i].fd == listenfd) {
          // a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
          // pe care serverul o accepta
          struct sockaddr_in cli_addr;
          socklen_t cli_len = sizeof(cli_addr);
          int newsockfd =
              accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
          DIE(newsockfd < 0, "accept");

          struct chat_packet recv_packet; // aici ma folosesc de structura din lab 7
          // ca sa primesc conform 
          int rc1 = recv_all(newsockfd, &recv_packet, sizeof(recv_packet));
          DIE(rc1 < 0, "No subscriber ID\n");

          // se adauga noul socket intors de accept() la multimea descriptorilor
          // de citire
          int exists = 0;
          char new_id[20] = {0};
          strcpy(new_id, recv_packet.message);

          for (int ii = 0; exists == 0 && ii< num_clients; ++ii) {
            if (strcmp(new_id, id_map_topics[ii].id) == 0) {
              exists = 1;
            }
          }

          if (exists == 1) {
            printf ("Client %s already connected.\n", new_id);
            // trb sa ii si dau mesaj de kill
            // o sa ii trimit mesaj de tip udp si la tip o sa fac o conventie si 
            // el o sa se inchida
            struct udp_msg kill_msg;
            memset(&kill_msg, 0, sizeof(struct udp_msg));
            uint8_t tip_kill = ~0;
            kill_msg.type = tip_kill;

            rc = send_all(newsockfd, &kill_msg, sizeof(struct udp_msg));         
            DIE(rc < 0, "Eroare la trimitere kill");

            close(newsockfd); // am inchis socketul abia deschis
          } else {

            poll_fds[num_clients].fd = newsockfd;
            poll_fds[num_clients].events = POLLIN;

            // trb sa pastrez acuma id ul astuia al meu !!!!!!!!!!!!!
            // ceva inteligent
            // aceeasi indice cu asta de poll_fd
            strcpy(id_map_topics[num_clients].id, recv_packet.message);
            id_map_topics[num_clients].num_topics = 0; // de fapt 0 - asta e o margine superioara

            num_clients++;
            printf("New client %s connected from %s:%d.\n", recv_packet.message,
             inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            // acuma mai trebuie verificat daca a mai fost logat inainte si 
            // trb sa ii trimit restanta de pachete

            int w = -1;
            for (int t = 0; t < total_number_ids; ++t) {
              if (strcmp(recv_packet.message, id_unsent_msgs[t].id) == 0) {
                w = t;
              }
            }
            if (w == -1) {
              strcpy(id_unsent_msgs[total_number_ids].id, recv_packet.message);
              total_number_ids++;
            } else {
              // ii trimit mesajele cat timp a fost absent
              // alea din sf1
              int no = id_unsent_msgs[w].num_unsent;
              //printf("Nr pachete restante pt Gigelul asta %d\n", no);
              for (int y = 0; y < no; ++y) {
                send_all(newsockfd, &(id_unsent_msgs[w].unsent_msgs_sf1[y]), sizeof(struct udp_msg));
              }
              memset(id_unsent_msgs[w].unsent_msgs_sf1, 0, 50 * sizeof(struct udp_msg));
              id_unsent_msgs[w].num_unsent = 0;
            }


          }

        } else if (poll_fds[i].fd == udp_sock) {

          char buf_helper[1600] = {0};
          rc = recvfrom(udp_sock, buf_helper, sizeof(buf_helper), 0, (struct sockaddr *) (& udp_addr),
           (socklen_t *)(&socket_len));
          DIE(rc < 0, "recv error from udp socket");

          // Aici partea mai tricky trebuie sa trimit doar la cei abonati la topicul meu
          // de inlocuit for ul asta cu ceva mai complicat ;(
          // A mai ramas partea asta
          struct udp_msg mesaj_udp;
          memset(&mesaj_udp, 0, sizeof(struct udp_msg));
          memcpy(&mesaj_udp, buf_helper, sizeof(struct udp_msg) - sizeof(uint16_t) - sizeof(struct in_addr));
          memcpy(&(mesaj_udp.curr_port), &(udp_addr.sin_port), sizeof(uint16_t));
          memcpy(&(mesaj_udp.curr_ip_addr), &(udp_addr.sin_addr), sizeof(struct in_addr));

          int rr = -1;
          for (int r = 0; r < num_topics; ++r) {
            if (strcmp(topic_map_id[r].topic, mesaj_udp.topic) == 0) {
              rr = r;
              break;
            }
          }
          if (rr == -1) {
            continue;
          }
          // pt sf0 daca e activ
          for (int t = 0; t < topic_map_id[rr].len_sf0; ++t) {
            char id_curr[20] = {0};
            strcpy(id_curr, (topic_map_id[rr].ids_sf0[t]));
            for (int j = 0; j < num_clients; ++j) {
              if (j != i && listenfd != poll_fds[j].fd && STDIN_FILENO != poll_fds[j].fd) {
                if (strcmp(id_map_topics[j].id, id_curr) == 0) {
                  send_all(poll_fds[j].fd, &mesaj_udp, sizeof(struct udp_msg));
                }
              }
            }
          }
          // pt sf1 daca e activ sau daca e abonat stocam
          for (int t = 0; t < topic_map_id[rr].len_sf1; ++t) {
            char id_curr[20] = {0};
            strcpy(id_curr, topic_map_id[rr].ids_sf1[t]);
            int found_active = 0;

            for (int j = 0; j < num_clients; ++j) {
              if (j != i && listenfd != poll_fds[j].fd && STDIN_FILENO != poll_fds[j].fd) {
                if (strcmp(id_map_topics[j].id, id_curr) == 0) {
                  found_active = 1;
                  send_all(poll_fds[j].fd, &mesaj_udp, sizeof(struct udp_msg));
                }
              }
            }

            if (found_active == 0) {
              int idx_info_unsent = 0;
              for (int z = 0; z < total_number_ids; ++z) {
                if (strcmp(id_curr, id_unsent_msgs[z].id) == 0) {
                  idx_info_unsent = z;
                  break;
                }
              }
              int no_help = id_unsent_msgs[idx_info_unsent].num_unsent;
              memcpy(& (id_unsent_msgs[idx_info_unsent].unsent_msgs_sf1[no_help]),
               &mesaj_udp, sizeof(struct udp_msg));
              id_unsent_msgs[idx_info_unsent].num_unsent++;
            }
          }


        } else if (poll_fds[i].fd == STDIN_FILENO) {
          char buffy[100] = {0};
          fgets(buffy, 100, stdin);

          if (strncmp(buffy, "exit", 4) == 0) {
            loop = 0;

            struct udp_msg kill_msg;
            memset(&kill_msg, 0, sizeof(struct udp_msg));
            uint8_t tip_kill = 254;
            kill_msg.type = tip_kill;

            //printf("Numarul clienti inainte sa imi dau kill %d\n", num_clients);
            // trimit mesaj de kill la toti clientii
            for (int j = 4; j < num_clients; ++j) {
              if (j != i && listenfd != poll_fds[j].fd) {
                rc = send_all(poll_fds[j].fd, &kill_msg, sizeof(struct udp_msg));         
                DIE(rc < 0, "Eroare la trimitere kill");

              }
            }

            for (int y = 0; y < num_clients; ++y)
              close(poll_fds[y].fd);

            // de asemenea eliberam ce am alocat dinamic inainte sa iesim
            free(poll_fds);
            free(id_map_topics);
            free(topic_map_id);
            free(id_unsent_msgs);

            break;
          } else {
            printf("Unrecognized command\n");
          }
        } else {

          // s-au primit date pe unul din socketii de client,
          // asa ca serverul trebuie sa le receptioneze
          // sa si zeroizez pachetul
          struct tcp_msg msgg_tcp;
          memset(&msgg_tcp, 0, sizeof(msgg_tcp));

          rc = recv_all(poll_fds[i].fd, &msgg_tcp, sizeof(msgg_tcp));
          DIE(rc < 0, "recv");

          if (rc == 0) {
            // conexiunea s-a inchis
            printf("Socket-ul client %d a inchis conexiunea\n", i);
            close(poll_fds[i].fd);

            // se scoate din multimea de citire socketul inchis
            for (int j = i; j < num_clients; j++) {
              // scot pe langa socket si map-ul meu
              poll_fds[j] = poll_fds[j + 1];
              id_map_topics[j] = id_map_topics[j + 1];
            }

            num_clients--;

          } else {

            if (msgg_tcp.type == 'e') {
              // atunci ala a dat rage quit si s-a deconectat
              int rc2 = close(poll_fds[i].fd);
              DIE(rc2 < 0, "Error disconnecting");

              // il scoatem si din multimea socketilor
              for (int j = i; j < num_clients; j++) {
                poll_fds[j] = poll_fds[j + 1];
                id_map_topics[j] = id_map_topics[j + 1];
              }
              num_clients--;

              printf("Client %s disconnected.\n", msgg_tcp.data);
              continue;

            } else if (msgg_tcp.type == 's') {
                // Aici de tip s, subscribe
                
                // hai sa facem abonarea
                char subject[100] = {0};
                strcpy(subject, msgg_tcp.data);
                strcpy (id_map_topics[i].topics[num_topics], subject);
                id_map_topics[i].num_topics++;

              int jj = -1;
              for (int j = 0; j < num_topics; ++j) {
                if (strcmp(subject, topic_map_id[j].topic) == 0) {
                  jj = j;
                  break;
                }
              }
              if (msgg_tcp.sf == 0) {
              if (jj == -1) {
                // adaugam acest topic
                jj = num_topics;

                strcpy(topic_map_id[num_topics].topic, subject);
                num_topics++;
                strcpy (topic_map_id[jj].ids_sf0[topic_map_id[jj].len_sf0], id_map_topics[i].id);
                topic_map_id[jj].len_sf0++;

              } else {
                // adaugam doar id - ul in jj deja gasit
                strcpy (topic_map_id[jj].ids_sf0[topic_map_id[jj].len_sf0], id_map_topics[i].id);
                topic_map_id[jj].len_sf0++;

              }
              } else {
                // aici sf1
                if (jj == -1) {
                // adaugam acest topic
                jj = num_topics;
                strcpy(topic_map_id[num_topics].topic, subject);
                num_topics++;
                strcpy (topic_map_id[jj].ids_sf1[topic_map_id[jj].len_sf1], id_map_topics[i].id);
                topic_map_id[jj].len_sf1++;

              } else {
                // adaugam doar id - ul in jj deja gasit
                strcpy (topic_map_id[jj].ids_sf1[topic_map_id[jj].len_sf1], id_map_topics[i].id);
                topic_map_id[jj].len_sf1++;

              }
              }
              // am actualizat tot cu subscribe

            } else {
                // aici sigur e de tip u -> unsubscribe
                char buffy[100] = {0};
                strcpy(buffy, msgg_tcp.data);
                // acuma din asta al lui id scot topic ul si le decrementez numarul
                //printf("Unsubscribe from %s\n", msgg_tcp.data);
                int nr = id_map_topics[i].num_topics;
                int jj = -1;
                for (int j = 0; j < nr; j++) {
                  if (strcmp(id_map_topics[i].topics[j], buffy) == 0) {
                    jj = j;
                    break;
                  }
                }

                if (jj != -1) {
                for (int j = jj; j < nr - 1; j++) {
                  strcpy(id_map_topics[i].topics[j], id_map_topics[i].topics[j + 1]);
                }

                id_map_topics[i].num_topics--;
                //si mai am si din structura inversa sa dezabonez
                for (int j = 0; j < num_topics; j++) {
                  if (strcmp(topic_map_id[j].topic, buffy) == 0) {
                    jj = j;
                    break;
                  }
                }

                int kk = -1;
                for (int k = 0; k < topic_map_id[jj].len_sf0; ++k) {
                  if (strcmp(buffy, topic_map_id[jj].ids_sf0[k]) == 0) {
                    kk = k;
                    break;
                  }
                }
                if (kk != -1) {
                for (int k = kk; k < topic_map_id[jj].len_sf0 - 1; ++k) {
                  strcpy(topic_map_id[jj].ids_sf0[k], topic_map_id[jj].ids_sf0[k + 1]);
                }
                topic_map_id[jj].len_sf0--;
                }

                kk = -1;
                for (int k = 0; k < topic_map_id[jj].len_sf1; ++k) {
                  if (strcmp(buffy, topic_map_id[jj].ids_sf1[k]) == 0) {
                    kk = k;
                    break;
                  }
                }
                if (kk != -1) {
                for (int k = kk; k < topic_map_id[jj].len_sf1 - 1; ++k) {
                  strcpy(topic_map_id[jj].ids_sf1[k], topic_map_id[jj].ids_sf1[k + 1]);
                }
                topic_map_id[jj].len_sf1--;
                }
                // am dezabonat
              }
                
                // am terminat cu unsubscribe ul

            }
          }
        }
      }
    }
  }
}

int main(int argc, char *argv[]) {

  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  if (argc != 2) {
    printf("\n Usage: %s <ip> <port>\n", argv[0]);
    return 1;
  }

  // Parsam port-ul ca un numar
  uint16_t port;
  int rc = sscanf(argv[1], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  // Obtinem un socket TCP pentru receptionarea conexiunilor
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(listenfd < 0, "socket");

  // Socket ptr UDP !!!!!
  int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
  DIE(udp_sock < 0, "socket UDP");



  // Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
  // rulam de 2 ori rapid
  int enable = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

  // Completăm in serv_addr adresa serverului, familia de adrese si portul
  // pentru conectare
  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  // Asociem adresa serverului cu socketul creat folosind bind
  rc = bind(listenfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "bind pe listenfd");

  // !!!! Neaparat deactivez algoritmul lui Naggle si am inclus si netinet/tcp.h
  setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, sizeof(int));
  setsockopt(udp_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, sizeof(int));


  // !!! Si setam si upd_addr
  memset(&udp_addr, 0, socket_len);
  udp_addr.sin_family = AF_INET;
  udp_addr.sin_port = htons(port);
  udp_addr.sin_addr.s_addr = INADDR_ANY;
  // Aici asociem partea de UDP!!
  rc = bind(udp_sock, (const struct sockaddr *)&udp_addr, sizeof(udp_addr));
  DIE(rc < 0, "bind esuat");

  run_chat_multi_server(listenfd, udp_sock);

  // Inchidem listenfd
  close(listenfd);
  close(udp_sock);
  return 0;
}
