TARGET: server

CC	= cc
CFLAGS	= -Wall -Wextra -O2 -std=c11
LFLAGS	= -Wall

server.o err.o: err.h

server: server.o err.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean TARGET
clean:
	rm -f server *.o *~ *.bak
