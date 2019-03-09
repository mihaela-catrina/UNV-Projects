#include "client.h"

#define BUFLEN 256
#define KGRN  "\x1B[32m"
#define RESET "\033[0m"

void error(char *msg) {
    perror(msg);
    exit(0);
}

//afiseaza un promt al clientului
//folosit pentru lizibilitate
void show_prompt() {
    printf(KGRN "client > " RESET);
    fflush(stdout);
}

//deschidere fisier de log pentru clientul cu pidul "pid"
FILE *open_file() {
    int pid = getpid();
    char filename[100];
    memset(filename, 0, strlen(filename));
    sprintf(filename, "%s%d%s", "client-", pid, ".log");
    FILE *file = fopen(filename, "a");
    if (file < 0) {
        puts("This file can't be open");
        exit(1);
    }
    return file;
}

//scriere in fisierul de log
void write_log_file(FILE *file, char *message, int fd) {
    //mesajul nu e comanda de la tastatura
    if (fd != STDIN)
        printf("%s", message);
    fprintf(file, "%s", message);
};

int main(int argc, char *argv[]) {
    ssize_t n;
    int sockfdTCP, sockfdUDP;
    int old_code = 0;
    // numarul ultimului card incercat cu comanda login
    char *last_no_card = NULL;
    char buffer[BUFLEN], reply[BUFLEN];
    // clientul are deja o sesiune deschisa pe server
    int connection = UNCONNECTED;
    struct sockaddr_in serv_addr_tcp;
    struct sockaddr_in serv_addr_udp;

    fd_set read_fds;    // multimea de citire folosita users_data_file select()
    fd_set tmp_fds;     // multime folosita temporar
    int fdmax;          // valoare maxima file descriptor din multimea read_fds

    // golim multimea de descriptori de citire (read_fds) si multimea tmp_fds
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    if (argc < 3) {
        fprintf(stderr, "Usage %s server_address server_port\n", argv[0]);
        exit(0);
    }

    sockfdTCP = socket(AF_INET, SOCK_STREAM, 0);
    sockfdUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfdTCP < 0)
        error("ERROR opening TCP socket");
    if (sockfdUDP < 0)
        error("ERROR opening UDP socket");
    // introducere socket TCP, UDP, STDIN in multimea de citire
    FD_SET(sockfdTCP, &read_fds);
    FD_SET(sockfdUDP, &read_fds);
    FD_SET(0, &read_fds);
    if (sockfdTCP > sockfdUDP)
        fdmax = sockfdTCP;
    else fdmax = sockfdUDP;

    // completare sockaddr_in pentru tcp
    serv_addr_tcp.sin_family = AF_INET;
    serv_addr_tcp.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr_tcp.sin_addr);

    //conectare prin TCP
    if (connect(sockfdTCP, (struct sockaddr *) &serv_addr_tcp, sizeof(serv_addr_tcp)) < 0)
        error("ERROR connecting TCP");

    // completare sockaddr_in pentru udp
    serv_addr_udp.sin_family = AF_INET;
    serv_addr_udp.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr_udp.sin_addr);

    show_prompt();

    //deschidere fisier de log pentru client
    FILE *file = open_file();
    while (1) {
        tmp_fds = read_fds;
        if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
            error("ERROR users_data_file select");
        // socketul pentru stdin => citire de la tastatura
        if (FD_ISSET(0, &tmp_fds)) {
            memset(buffer, 0, BUFLEN);
            fgets(buffer, BUFLEN - 1, stdin);
            if (strlen(buffer) < 2) continue;
            show_prompt();
            write_log_file(file, buffer, STDIN);
            //verifica comanda de la tastatura
            int code = check_command(buffer, &last_no_card);
            // pentru orice comanda ce necesita conectare, clientul fiind UNCONNECTED
            if (need_login_before(code) && connection == UNCONNECTED) {
                write_log_file(file, "-1: Clientul nu este autentificat\n", 1);
                fflush(stdout);
                show_prompt();
                // comada cunoscuta sau sirule specifice pentru UNLOCK (parola) si transfer(confirmare)
            } else if (code != UNKNOWN_COMMAND || old_code == UNLOCK || old_code == TRANSFER) {
                // comanda de login, clientul fiind deja conectat
                if (code == LOGIN && connection == CONNECTED) {
                    write_log_file(file, "-2 Sesiune deja deschisa\n", 1);
                    fflush(stdout);
                    show_prompt();
                    //unlock sau parola pentru unlock
                } else if (code == UNLOCK || (code == UNKNOWN_COMMAND && old_code == UNLOCK)) {
                    // trimite parola de unlock
                    if (old_code == UNLOCK) {
                        char *temp = strdup(buffer);
                        sprintf(buffer, "%s %s", last_no_card, temp);

                    } else {
                        if (last_no_card == NULL) error("You should have login before");
                        strcpy(buffer + 6, " ");
                        strcpy(buffer + 7, last_no_card);
                    }
                    fflush(stdout);
                    //trimite pe socketul de UDP
                    n = sendto(sockfdUDP, buffer, strlen(buffer), 0, (struct sockaddr *) &serv_addr_udp,
                               sizeof(serv_addr_udp));
                    if (n < 0)
                        error("ERROR writing to socket");
                } else if (code == LOGIN) {
                    n = send(sockfdTCP, buffer, strlen(buffer), 0);
                    if (n < 0)
                        error("ERROR writing to socket");
                } else if (code == LOGOUT) {
                    n = send(sockfdTCP, buffer, strlen(buffer), 0);
                    if (n < 0)
                        error("ERROR writing to socket");
                    // comanda de quit din client
                } else if (code == QUIT) {
                    n = send(sockfdTCP, buffer, strlen(buffer), 0);
                    if (n < 0)
                        error("ERROR writing to socket");
                    FD_CLR(sockfdTCP, &read_fds);
                    FD_CLR(sockfdUDP, &read_fds);
                    close(sockfdTCP);
                    close(sockfdUDP);
                    printf("Clientul se inchide\n");
                    return 0;
                } else if (code == LISTSOLD) {
                    n = send(sockfdTCP, buffer, strlen(buffer), 0);
                    if (n < 0)
                        error("ERROR writing to socket");
                    // transfer sau confirmare transfer
                } else if (code == TRANSFER || (code == UNKNOWN_COMMAND && old_code == TRANSFER)) {
                    n = send(sockfdTCP, buffer, strlen(buffer), 0);
                    if (n < 0)
                        error("ERROR writing to socket");
                }
            } else if (code == UNKNOWN_COMMAND) {
                write_log_file(file, "-10 Comanda incorecta\n", 1);
                fflush(stdout);
                show_prompt();
            }
            old_code = code;
            // primire pe socketul de TCP
        } else if (FD_ISSET(sockfdTCP, &tmp_fds)) {
            memset(reply, 0, sizeof(reply));
            read(sockfdTCP, reply, BUFLEN);
            // se inchide serverul
            if (strcmp(reply, "server quit") == 0) {
                printf("Serverul s-a inchis\n");
                FD_CLR(sockfdTCP, &read_fds);
                FD_CLR(sockfdUDP, &read_fds);
                close(sockfdTCP);
                close(sockfdUDP);
                return 0;
            }
            if (strcmp(reply, "-3") == 0) {
                write_log_file(file, "IBANK > -3 PIN incorect\n", 1);
            } else if (strcmp(reply, "-4") == 0) {
                write_log_file(file, "IBANK > -4 Numar card incorect\n", 1);
            } else if (strcmp(reply, "-2") == 0) {
                write_log_file(file, "IBANK > -2 Sesiune deja deschisa\n", 1);
            } else if (strcmp(reply, "-5") == 0) {
                write_log_file(file, "IBANK > -5 Card blocat\n", 1);
            } else if (strcmp(reply, "-8") == 0) {
                write_log_file(file, "IBANK > -8 Fonduri insuficiente\n", 1);
            } else if (strcmp(reply, "-9") == 0) {
                write_log_file(file, "IBANK > -9 Operatie anulata\n", 1);
                //deconectare client => connection = UNCONNECTED
            } else if ((strcmp(reply, "Clientul a fost deconectat") == 0)) {
                connection = UNCONNECTED;
                char *message = (char *) calloc(strlen(reply) + 10, sizeof(char));
                sprintf(message, "%s %s %s", "IBANK > ", reply, "\n");
                write_log_file(file, message, 1);
                free(message);
                // orice alt mesaj de pe socketul de TCP
            } else {
                connection = CONNECTED;
                char *message = (char *) calloc(strlen(reply) + 10, sizeof(char));
                sprintf(message, "%s %s %s", "IBANK > ", reply, "\n");
                write_log_file(file, message, 1);
                free(message);
            }
            show_prompt();
            // am primit mesaj pe socketul de UDP
        } else if (FD_ISSET(sockfdUDP, &tmp_fds)) {
            memset(reply, 0, sizeof(reply));
            int recv_len = sizeof(serv_addr_udp);
            recvfrom(sockfdUDP, reply, BUFLEN, 0, (struct sockaddr *) &serv_addr_udp, &recv_len);
            if (strcmp(reply, "Trimite") == 0) {
                write_log_file(file, "UNLOCK > Trimite parola secreta\n", 1);
            } else if (strcmp(reply, "-7") == 0) {
                write_log_file(file, "UNLOCK > Deblocare esuata\n", 1);
            } else if (strcmp(reply, "-4") == 0) {
                write_log_file(file, "UNLOCK > -4 Numar card incorect\n", 1);
            } else if (strcmp(reply, "-6") == 0) {
                old_code = 0;
                write_log_file(file, "UNLOCK > -6 Operatie esuata\n", 1);
            } else printf("UNLOCK> %s\n", reply);
            show_prompt();
        }
    }
}
