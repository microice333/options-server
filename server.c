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

char negotiation[] = {IAC, DO, TELOPT_LINEMODE};
char buf2[] = {IAC, SB, TELOPT_LINEMODE, TELOPT_ECHO, TELQUAL_SEND, 255, 240};
// char NEG[] = {255, 251, 1};
char NEG[] = {IAC, SB, TELOPT_DET, 8, IAC, SE};
char NEG2[] ={IAC, SB, TELOPT_DET, 5, 1, 0, IAC, SE};
char clear_terminal[] = "\e[1;1H\e[2J";
char CLEAR_SIGN[]={0x1B,'c','\0'};

char menu_chosen_a[] = "-->Opcja A\n   Opcja B\n   Koniec\n";
char menu_chosen_b[] = "   Opcja A\n-->Opcja B\n   Koniec\n";
char menu_chosen_end[] = "   Opcja A\n   Opcja B\n-->Koniec\n";

char UP_ARROW[] = {27, 91, 65};
char DOWN_ARROW[] = {27, 91, 66};

char buffer[BUFFER_SIZE];

bool is_number(char *arr) {
    int res = 1;

    int len = strlen(arr);

    for (int i = 0; i< len && res != 0; i++) {
        res = isdigit(arr[i]);
    }

    return res == 0 ? false : true;
}

bool is_up_arrow(ssize_t len) {
    return len == 3 && buffer[0] == UP_ARROW[0] && buffer[1] == UP_ARROW[1] 
            && buffer[2] == UP_ARROW[2];
}

bool is_down_arrow(ssize_t len) {
    return len == 3 && buffer[0] == DOWN_ARROW[0] && buffer[1] == DOWN_ARROW[1] 
            && buffer[2] == DOWN_ARROW[2];
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

    for (;;) {
        client_address_len = sizeof(client_address);
        msg_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_len);

        if (msg_sock < 0)
            syserr("Accept");

        int curr_option = 0;
        snd_len = write(msg_sock, negotiation, sizeof(negotiation));
        // snd_len = write(msg_sock, menu_chosen_a, sizeof(menu_chosen_a));

        do {
            snd_len = write(msg_sock, clear_terminal, sizeof(clear_terminal));

            if (curr_option == 0) {
                snd_len = write(msg_sock, menu_chosen_a, sizeof(menu_chosen_a));
            } else if (curr_option == 1) {
                snd_len = write(msg_sock, menu_chosen_b, sizeof(menu_chosen_b));
            } else {
                snd_len = write(msg_sock, menu_chosen_end, sizeof(menu_chosen_end));
            }

            len = read(msg_sock, buffer, sizeof(buffer));
            // write(1, "du[a", 4);
            // if (len < 0)
                // syserr("Reading from client socker");

            // printf("%d %d %d %d\n", len, sizeof(UP_ARROW), strlen(UP_ARROW), sizeof(DOWN_ARROW));
            // // char xd[3];
            // // strncpy(xd, buffer, 3);

            if (is_up_arrow(len)) {
                curr_option = curr_option == 0 ? 2 : curr_option - 1;
            } else if (is_down_arrow(len)) {
                curr_option++;
                curr_option = curr_option % 3;
            }
        } while (len > 0);
        
        printf("ending connection\n");
        
        if (close(msg_sock) < 0)
            syserr("Close");
    }

    return 0;
}
