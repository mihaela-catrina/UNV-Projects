#include "server.h"


void error(char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {

    if (argc < 3) {
        fprintf(stderr, "Usage : %s port users_data_file\n", argv[0]);
        exit(1);
    }

    int sockfdTCP, newsockfd, portno, clilen, max_clients, no_bytes, sockfdUDP;
    struct sockaddr_in serv_addr_TCP, cli_addr;
    struct sockaddr_in serv_addr_UDP, from_station;
    char buffer[BUFLEN], *pin, *no_card, *password;
    //vectorul de clienti
    clientData *clients;
    // citire din users_data_file
    FILE *fd = fopen(argv[2], "r");
    clients = read_clients_info(fd, &max_clients);
    // vector in care se retin posibilele transferuri in desfasurare de la clienti
    // folosit la transfer dupa confirmare
    transfer transferuri[100];
    for (int i = 0; i < 100; i++) {
        transferuri[i].destination_idx = -1; // destinatar
        transferuri[i].source_idx = -1;      // sursa
        transferuri[i].sum = 0;             // suma transferata
    }

    fd_set read_fds;     //multimea de citire folosita users_data_file select()
    fd_set tmp_fds;      //multime folosita temporar
    int fdmax;           //valoare maxima file descriptor din multimea read_fds

    //golim multimea de descriptori de citire (read_fds) si multimea tmp_fds
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    /*Deschidere socket TCP*/
    sockfdTCP = socket(AF_INET, SOCK_STREAM, 0);
    /*Deschidere socket UDP*/
    sockfdUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfdTCP < 0)
        error("ERROR opening socket");
    if (sockfdUDP < 0)
        error("ERROR opening UDP socket");

    portno = atoi(argv[1]);
    // completare informatii despre adresa serverului
    memset((char *) &serv_addr_TCP, 0, sizeof(serv_addr_TCP));
    serv_addr_TCP.sin_family = AF_INET;
    serv_addr_TCP.sin_addr.s_addr = INADDR_ANY;
    serv_addr_TCP.sin_port = htons(portno);

    if (bind(sockfdTCP, (struct sockaddr *) &serv_addr_TCP, sizeof(struct sockaddr)) < 0)
        error("ERROR on binding TCP");

    listen(sockfdTCP, max_clients);

    /*Setare struct sockaddr_in pentru a asculta pe portul de UDP */
    memset((char *) &serv_addr_UDP, 0, sizeof(serv_addr_UDP));
    serv_addr_UDP.sin_family = AF_INET;
    serv_addr_UDP.sin_port = htons(atoi(argv[1]));
    serv_addr_UDP.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Legare proprietati de socket */
    if (bind(sockfdUDP, (struct sockaddr *) &serv_addr_UDP, sizeof(serv_addr_UDP))) {
        error("ERROR on binding UDP");
    }

    // adaugam noii file descriptori
    FD_SET(sockfdTCP, &read_fds);
    FD_SET(sockfdUDP, &read_fds);
    if (sockfdTCP > sockfdUDP)
        fdmax = sockfdTCP;
    else fdmax = sockfdUDP;
    FD_SET(0, &read_fds);

    while (1) {
        tmp_fds = read_fds;
        if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
            error("ERROR users_data_file select");
        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                // a venit ceva pe socketul de ascultare TCP = o noua conexiune
                if (i == sockfdTCP) {
                    // actiunea serverului: accept()
                    clilen = sizeof(cli_addr);
                    if ((newsockfd = accept(sockfdTCP, (struct sockaddr *) &cli_addr, (socklen_t *) &clilen)) == -1) {
                        error("ERROR users_data_file accept");
                    } else {
                        //adaug noul socket intors de accept() la multimea
                        //descriptorilor de citire
                        FD_SET(newsockfd, &read_fds);
                        if (newsockfd > fdmax)
                            fdmax = newsockfd;
                    }
                    printf("Noua conexiune de la %s, port %d, socket_client %d\n ", inet_ntoa(cli_addr.sin_addr),
                           ntohs(cli_addr.sin_port), newsockfd);
                    // a venit ceva pe socketul de ascultare UDP
                } else if (i == sockfdUDP) {
                    memset(buffer, 0, BUFLEN);
                    int recv_len = sizeof(from_station);
                    recvfrom(sockfdUDP, buffer, BUFLEN, 0, (struct sockaddr *) &from_station,
                             &recv_len);
                    printf("Am primit mesaj pe socketul de UDP\n");
                    //verific comanda primita
                    int code = get_command(buffer, &no_card, &password);
                    // pot primi doar UNLOCK pe acest socket
                    if (code == UNLOCK) {
                        // extragere numar card
                        long int numar_card = strtol(no_card, NULL, 10);
                        // clientul pentru care se da unlock
                        int client_pos = is_client_in_data_base(clients, max_clients, numar_card);
                        // nu exista clientul in baza de date
                        if (client_pos < 0) {
                            memset(buffer, 0, strlen(buffer));
                            strcpy(buffer, "-4");
                            sendto(i, buffer, BUFLEN, 0, (struct sockaddr *) &from_station, sizeof(from_station));

                        } else {
                            // se incearca deblocarea
                            unlock_card(buffer, &clients[client_pos]);
                            sendto(i, buffer, BUFLEN, 0, (struct sockaddr *) &from_station, sizeof(from_station));
                        }
                    }
                    //se citeste de la tastatura
                } else if (i == 0) {
                    memset(buffer, 0, strlen(buffer));
                    fgets(buffer, BUFLEN - 1, stdin);
                    // pot primi doar "QUIT" de la tastatura
                    if (strcmp(buffer, "quit\n") == 0) {
                        // Se inchide serverul
                        printf("Serverul se inchide.\n");
                        for (int j = 0; j <= fdmax; j++) {
                            if (j != sockfdTCP && j != 0) {
                                //anunta clientii
                                memset(buffer, 0, strlen(buffer));
                                strcpy(buffer, "server quit");
                                send(j, buffer, strlen(buffer) + 1, 0);
                                //inchide conexiunea
                                FD_CLR(j, &read_fds);
                                close(j);
                            }
                        }
                        FD_CLR(sockfdTCP, &read_fds);
                        close(sockfdTCP);
                        FD_CLR(sockfdUDP, &read_fds);
                        close(sockfdUDP);
                        return 0;
                    } else {
                        printf("Comanda necunoscuta.\n");
                        fflush(stdin);
                        fflush(stdout);
                        return 0;
                    }
                    // am primit mesaje de la clienti
                } else {
                    memset(buffer, 0, BUFLEN);
                    if ((no_bytes = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
                        if (no_bytes == 0) {
                            // conexiunea s-a inchis
                            printf("server: socket %d hung up\n", i);
                        } else {
                            error("ERROR users_data_file recv");
                        }
                        close(i);
                        FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul
                    } else {
                        printf("Am primit de la clientul de pe socketul %d un mesaj.\n", i);
                        // verific comanda primita
                        int code = get_command(buffer, &no_card, &pin);
                        // cerere de LOGIN
                        if (code == LOGIN) {
                            long int numar_card = strtol(no_card, NULL, 10);
                            long int _pin = strtol(pin, NULL, 10);
                            // verificare numar card
                            int client_pos = is_client_in_data_base(clients, max_clients, numar_card);
                            if (client_pos < 0) {
                                memset(buffer, 0, strlen(buffer));
                                strcpy(buffer, "-4");
                                send(i, buffer, strlen(buffer) + 1, 0);
                            } else {
                                // verificare client deja conectat
                                int connection = verify_already_connected_client(buffer, clients[client_pos], i);
                                // client neconectat => verificare pin
                                if (connection == UNCONNECTED)
                                    verify_pin_code(buffer, &clients[client_pos], i, _pin);
                            }
                        }
                        // mesaj de QUIT de la client
                        if (code == QUIT) {
                            // deconectare de la server a clientului
                            int client_pos = search_associated_client(clients, max_clients, i);
                            clients[client_pos].connected = UNCONNECTED;
                            FD_CLR(i, &read_fds);
                            close(i);
                        }
                        // cerere de LOGOUT
                        if (code == LOGOUT) {
                            // verificare client asociat cu file descriptorul de pe care se primeste
                            // mesajul de logout
                            int client_pos = search_associated_client(clients, max_clients, i);
                            // deconectare
                            clients[client_pos].connected = UNCONNECTED;
                            clients[client_pos].associated_fd = 0;
                            memset(buffer, 0, strlen(buffer));
                            strcpy(buffer, "Clientul a fost deconectat");
                            send(i, buffer, strlen(buffer) + 1, 0);

                        }
                        // cerere de list sold
                        if (code == LISTSOLD) {
                            // verificare client asociat cu file descriptorul de pe care se primeste
                            // mesajul de listsold
                            int client_pos = search_associated_client(clients, max_clients, i);
                            memset(buffer, 0, strlen(buffer));
                            sprintf(buffer, "%.2f", clients[client_pos].sold);
                            send(i, buffer, strlen(buffer) + 1, 0);
                        }
                        // cerere de transfer
                        if (code == TRANSFER) {
                            double sum = 0;
                            sum = strtof(pin, 0);
                            long int numar_card = strtol(no_card, NULL, 10);
                            // verificare numar card
                            int client_pos = is_client_in_data_base(clients, max_clients, numar_card);
                            // verificare client asociat cu procesul curent
                            int source = search_associated_client(clients, max_clients, i);
                            // numar card necunoascut => destinatie necunoascuta
                            if (client_pos < 0) {
                                memset(buffer, 0, strlen(buffer));
                                strcpy(buffer, "-4");
                                send(i, buffer, strlen(buffer) + 1, 0);
                                // sold insuficient
                            } else if (clients[client_pos].sold < sum) {
                                memset(buffer, 0, strlen(buffer));
                                strcpy(buffer, "-8");
                                send(i, buffer, strlen(buffer) + 1, 0);
                                // trimitere mesaj de confirmare a transferului
                            } else {
                                memset(buffer, 0, strlen(buffer));
                                sprintf(buffer, "%s %s %s %s %s %s", "Transfer", pin, "catre",
                                        clients[client_pos].first_name, clients[client_pos].last_name, "?");
                                send(i, buffer, strlen(buffer) + 1, 0);
                                transferuri[i].destination_idx = client_pos;                    //transfer neconfirmat
                                transferuri[i].source_idx = source;
                                transferuri[i].sum = sum;
                            }
                        }
                        // comanda necunoascuta sau transfer in curs
                        if (code == UNKNOWN_COMMAND && transferuri[i].destination_idx != -1) {
                            // transfer realizat cu succes
                            if (buffer[0] == 'y') {
                                clients[transferuri[i].destination_idx].sold += transferuri[i].sum;
                                clients[transferuri[i].source_idx].sold -= transferuri[i].sum;
                                memset(buffer, 0, strlen(buffer));
                                strcpy(buffer, "Transfer realizat cu succes");
                                send(i, buffer, strlen(buffer) + 1, 0);
                                // anulare transfer
                            } else {
                                memset(buffer, 0, strlen(buffer));
                                strcpy(buffer, "-9");
                                send(i, buffer, strlen(buffer) + 1, 0);
                            }
                        }
                    }
                }
            }
        }
    }
}
