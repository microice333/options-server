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
char CLEAR_SIGN[]={0x1B,'c','\0'};


char NEG[] = "\377\375\042\377\373\001";

char *menu_chosen_a = "-->Opcja A\n   Opcja B\n   Koniec\n";
char *menu_chosen_b = "   Opcja A\n-->Opcja B\n   Koniec\n";
char *menu_chosen_end = "   Opcja A\n   Opcja B\n-->Koniec\n";

char UP_ARROW[] = {27, 91, 65, '\0'};
char DOWN_ARROW[] = {27, 91, 66, '\0'};

char buffer[BUFFER_SIZE];

bool is_number(char *arr) {
    int res = 1;

    int len = strlen(arr);

    for (int i = 0; i< len && res != 0; i++) {
        res = isdigit(arr[i]);
    }

    return res == 0 ? false : true;
}

int main(int argc, char *argv[]) {

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
    int curr_option = 0;

    for (;;) {
        client_address_len = sizeof(client_address);
        msg_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_len);
        int n = send(msg_sock, NEG, strlen(NEG), 0);
        // n = send(msg_sock, buf2, 7, 0);

        if (msg_sock < 0)
            syserr("Accept");

        snd_len = write(msg_sock, menu_chosen_a, strlen(menu_chosen_a));

        do {
            len = read(msg_sock, buffer, sizeof(buffer));
            char xd[3];
            strncpy(xd, buffer, 3);
            printf("%d %d\n", strlen(UP_ARROW), strlen(DOWN_ARROW));
            if (strcmp(xd, UP_ARROW) == 0) {
                curr_option = curr_option == 0 ? 2 : curr_option - 1;
            } else if (strcmp(xd, DOWN_ARROW) == 0) {
                curr_option++;
                curr_option = curr_option % 3;
            }

            n = send(msg_sock, CLEAR_SIGN, strlen(CLEAR_SIGN), 0);
            if (curr_option == 0) {
                snd_len = write(msg_sock, menu_chosen_a, strlen(menu_chosen_a));
            } else if (curr_option == 1) {
                snd_len = write(msg_sock, menu_chosen_b, strlen(menu_chosen_b));
            } else {
                snd_len = write(msg_sock, menu_chosen_end, strlen(menu_chosen_end));
            }
        } while (len > 0);
        
        printf("ending connection\n");
        
        if (close(msg_sock) < 0)
            syserr("Close");
    }

    return 0;
}
