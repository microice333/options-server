#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include "err.h"
#include <arpa/telnet.h>

#define BUFFER_SIZE   2000
#define QUEUE_LENGTH     5

char buf1[] = {255, 253, 34};
char buf2[] = {255, 250, 34, 1, 0, 255, 240};
char buf3[] = {255, 251, 1};

bool is_number(char *arr) {
    int res = 1;

    int len = strlen(arr);

    for (int i = 0; i< len && res != 0; i++) {
        res = isdigit(arr[i]);
    }

    return res == 0 ? false : true;
}

int main(int argc, char *argv[]) {
    char *menu = "-->Opcja A\n   Opcja B\n   Koniec\n";

    int menu_size = strlen(menu);

    char buffer[BUFFER_SIZE];

    if (argc != 2) {
        syserr("Wrong parameters number. Usage: %s port", argv[0]);
    }

    if (!is_number(argv[1])) {
        syserr("Parameter should be port number");
    }

    int port = atoi(argv[1]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        syserr("Socket");
    }

    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET; 
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_address.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
        syserr("Bind");

  
    if (listen(sock, QUEUE_LENGTH) < 0)
        syserr("Listen");

    ssize_t len, snd_len;
    socklen_t client_address_len;
    struct sockaddr_in client_address;
    int msg_sock;

    for (;;) {
        client_address_len = sizeof(client_address);
        msg_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_len);
        int n = send(msg_sock, buf1, 3, 0);
        n = send(msg_sock, buf2, 7, 0);
        n = send(msg_sock, buf3, 3, 0);

        if (msg_sock < 0)
            syserr("Accept");

        do {
            snd_len = write(msg_sock, menu, menu_size);
            len = read(msg_sock, buffer, sizeof(buffer));
            write(1, buffer, len);
        } while (len > 0);
        
        printf("ending connection\n");
        
        if (close(msg_sock) < 0)
            syserr("Close");
    }

    return 0;
}
