// Copyright Raducanu George-Cristian Tema 2 PCOM

#ifndef STRUCTURES_H
#define STRUCTURES_H

#define BUF_TOPIC 50
#define BUF_DATA 1500

#include <netinet/in.h>

// Structuri pt mesaje de tip UDP si TCP

typedef struct udp_msg {
    char topic[BUF_TOPIC];
    uint8_t type;
    char data[BUF_DATA];
    struct in_addr curr_ip_addr;
    uint16_t curr_port;
} udp_msg;

typedef struct tcp_msg {
    char type;
    char data[BUF_DATA];
    uint8_t sf;
} tcp_msg;

#define ID_SIZE 20
#define TOPIC_SIZE 100

// Aici in tructurile astea practic mimic desi mai ineficient
// Comportamentul unui bimap

// o sa am un vector ce coresp fiecarui id, o sa fie pe aceeasi pozitie cu 
// vectorul de poll_fd
typedef struct id_topics {
  char id[ID_SIZE];
  char topics[TOPIC_SIZE][TOPIC_SIZE];
  int num_topics;
} id_topics;

// Aici o sa tin minte fiecare topic si cui ii corespunde
typedef struct topic_ids {
  char topic[TOPIC_SIZE];
  char ids_sf0[TOPIC_SIZE][ID_SIZE];
  int len_sf0;
  char ids_sf1[TOPIC_SIZE][ID_SIZE];
  int len_sf1;
} topic_ids;

// Aici alta structura ajutatoare in cazul in care se deconecteaza si dupa
// vrea sa primeasca mesajele de la topicuri cu sf1

typedef struct info_unsent_id {
  char id[ID_SIZE];
  struct udp_msg unsent_msgs_sf1[50];
  int num_unsent;
}info_unsent_id;
#endif
