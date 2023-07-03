// Copyright Raducanu George-Cristian Tema 2 PCOM


// Credit laboratotul 7 de pcom !
#include "common.h"

#include <sys/socket.h>
#include <sys/types.h>

int recv_all(int sockfd, void *buffer, size_t len) {

  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  char *buff = (char *)buffer;

      while(bytes_remaining) {
        //  TODO: Make the magic happen
        int aux = recv(sockfd, buff + bytes_received, bytes_remaining, 0);
        if (aux == -1)
            return -1;
        
        bytes_received += aux;
        bytes_remaining -= aux;
      }

    // cat am primit
  return bytes_received;
}

int send_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = (char *)buffer;
      while(bytes_remaining) {
          //TODO: Make the magic happen
        int aux = send(sockfd, buff + bytes_sent, bytes_remaining, 0);
        if (aux == -1)
            return -1;
        bytes_sent += aux;
        bytes_remaining -= aux;
      }

  return bytes_sent;
}
