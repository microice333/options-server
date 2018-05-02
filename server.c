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
#define MAIN_MENU        0
#define B_MENU           1

//Negotiation options
char LINEMODE[] = {IAC, DO, TELOPT_LINEMODE};
char LINEMODE_SB[] = {IAC, SB, TELOPT_LINEMODE, LM_MODE, 0, IAC, SE};
char ECHO[] = {IAC, WILL, TELOPT_ECHO};

//Menu
char CLEAR_SIGN[]={0x1B,'c','\0'};
char FIRST_MENU[] = "Opcja A\r\nOpcja B\r\nKoniec\r\n";
char SECOND_MENU[] = "Opcja B1\r\nOpcja B2\r\nWstecz\r\n";
char CHOSEN_A[] = {0x1B,'[', '3', 'B','A', 0x1B,'[', '3', 'A', 0x1B,'[', 'D', 
                   '\0'};
char CHOSEN_B1[] = {0x1B,'[', '3', 'B', 'B', '1', 0x1B,'[', '3', 'A', 0x1B, 
                    '[', '2', 'D', '\0'};
char CHOSEN_B2[] = {0x1B,'[', '2', 'B', 'B', '2', 0x1B,'[', '2', 'A', 0x1B,
                    '[', '2', 'D', '\0'};

//Keys to work with
char UP_ARROW[] = {27, 91, 65};
char DOWN_ARROW[] = {27, 91, 66};
char ENTER[] = {13, 0};

//Moving cursor
char MOVE_UP[] = {0x1B,'[', 'A','\0'};
char MOVE_2_UP[] = {0x1B,'[', '2', 'A','\0'};
char MOVE_3_UP[] = {0x1B,'[', '3', 'A','\0'};
char MOVE_DOWN[] = {0x1B,'[', 'B','\0'};
char MOVE_2_DOWN[] = {0x1B,'[', '2', 'B','\0'};
char MOVE_3_DOWN[] = {0x1B,'[', '3', 'B','\0'};

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

bool is_enter(ssize_t len) {
    return len == 2 && buffer[0] == ENTER[0] && buffer[1] == ENTER[1];
}

void check_write(ssize_t snd_len, ssize_t len) {
    if (snd_len != len)
        syserr("Partial / failed write");
}

void set_cursor_on_begin(int msg_sock) {
    ssize_t snd_len;
    
    snd_len = write(msg_sock, MOVE_3_UP, sizeof(MOVE_3_UP));
    check_write(snd_len, sizeof(MOVE_3_UP));
}

void show_menu(int msg_sock, int menu) {
    ssize_t snd_len;

    snd_len = write(msg_sock, CLEAR_SIGN, sizeof(CLEAR_SIGN));
    check_write(snd_len, sizeof(CLEAR_SIGN));

    if (menu == MAIN_MENU) {
        snd_len = write(msg_sock, FIRST_MENU, sizeof(FIRST_MENU));
        check_write(snd_len, sizeof(FIRST_MENU));
    } else {
        snd_len = write(msg_sock, SECOND_MENU, sizeof(SECOND_MENU));
        check_write(snd_len, sizeof(SECOND_MENU));
    }

    set_cursor_on_begin(msg_sock);
}

void negotiate(int msg_sock) {
    ssize_t snd_len;

    snd_len = write(msg_sock, LINEMODE, sizeof(LINEMODE));
    check_write(snd_len, sizeof(LINEMODE));
    
    snd_len = write(msg_sock, LINEMODE_SB, sizeof(LINEMODE_SB));
    check_write(snd_len, sizeof(LINEMODE_SB));

    snd_len = write(msg_sock, ECHO, sizeof(ECHO));
    check_write(snd_len, sizeof(ECHO));
}

void move_cursor_up(int msg_sock, int *curent_option) {
    ssize_t snd_len;
    
    if (*curent_option == 0) {
        snd_len = write(msg_sock, MOVE_2_DOWN, sizeof(MOVE_2_DOWN));
        check_write(snd_len, sizeof(MOVE_2_DOWN));
        (*curent_option) = 2;
    } else {
        snd_len = write(msg_sock, MOVE_UP, sizeof(MOVE_UP));
        check_write(snd_len, sizeof(MOVE_UP));
        (*curent_option)--;
    }
}

void move_cursor_down(int msg_sock, int *current_option) {
    ssize_t snd_len;
    
    if (*current_option == 2) {
        snd_len = write(msg_sock, MOVE_2_UP, sizeof(MOVE_2_UP));
        check_write(snd_len, sizeof(MOVE_2_UP));
        (*current_option) = 0;
    } else {
        snd_len = write(msg_sock, MOVE_DOWN, sizeof(MOVE_DOWN));
        check_write(snd_len, sizeof(MOVE_DOWN));
        (*current_option)++;
    }
}

bool decide(int msg_sock, int *current_option, int *current_menu) {
    ssize_t snd_len;

    if (*current_menu == MAIN_MENU) {
        if (*current_option == 0) {
            snd_len = write(msg_sock, CHOSEN_A, sizeof(CHOSEN_A));
            check_write(snd_len, sizeof(CHOSEN_A));
        } else if (*current_option == 1) {
            show_menu(msg_sock, B_MENU);
            
            (*current_option) = 0;
            (*current_menu) = B_MENU;
        } else {
            return true;
        }
    } else {
        if (*current_option == 0) {
            snd_len = write(msg_sock, CHOSEN_B1, sizeof(CHOSEN_B1));
            check_write(snd_len, sizeof(CHOSEN_B1));
        } else if (*current_option == 1) {
            snd_len = write(msg_sock, CHOSEN_B2, sizeof(CHOSEN_B2));
            check_write(snd_len, sizeof(CHOSEN_B2));           
        } else {
            show_menu(msg_sock, MAIN_MENU);
            
            (*current_option) = 0;
            (*current_menu) = MAIN_MENU;
        } 
    }

    return false;
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

    ssize_t len;
    socklen_t client_address_len;
    struct sockaddr_in client_address;
    int msg_sock;

    for (;;) {
        client_address_len = sizeof(client_address);
        msg_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_len);

        if (msg_sock < 0)
            syserr("Accept");

        int curr_option = 0, curr_menu = 0;
        bool closed = false;

        negotiate(msg_sock);
        show_menu(msg_sock, MAIN_MENU);

        do {
            len = read(msg_sock, buffer, sizeof(buffer));

            if (len < 0)
                syserr("Reading from client socker");

            if (is_up_arrow(len)) {
                move_cursor_up(msg_sock, &curr_option);
            } else if (is_down_arrow(len)) {
                move_cursor_down(msg_sock, &curr_option);
            } else if (is_enter(len)) {
                closed = decide(msg_sock, &curr_option, &curr_menu);
            }
        } while (len > 0 && !closed);
        
        printf("Ending connection\n");
        
        if (close(msg_sock) < 0)
            syserr("Close");
    }

    return 0;
}
