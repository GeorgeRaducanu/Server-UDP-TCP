# Copyright Raducanu George-Cristian 321CAb 2022-2023

CFLAGS = -Wall -Wextra -Wpedantic -Wshadow -g

all: server subscriber

server: server.c common.o
	gcc $(CFLAGS) server.c common.o -o server

subscriber: subscriber.c common.o
	gcc $(CFLAGS) subscriber.c common.o -o subscriber

common.o : common.c

.PHONY: clean

clean: 
	rm -f server subscriber *.o
