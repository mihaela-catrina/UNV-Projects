//
// Created by mihaela on 02.05.2018.
//
#include "lib.h"


#ifndef TEMA2_SERVER_H
#define TEMA2_SERVER_H
#define BUFLEN 256

typedef struct clientData {
    char first_name[13];
    char last_name[13];
    int no_card;
    int pin;
    char password[9];
    double sold;
    unsigned char connected;
    unsigned char failed_attempts;      //incercari esuate de conectare
    int associated_fd;

} clientData;

typedef struct transfer {
    int destination_idx;
    int source_idx;
    double sum;
} transfer;

// citire date clienti din fisierul de input
clientData *read_clients_info(FILE *fd, int *no_clients) {
    int n;
    fscanf(fd, "%d", &n);
    clientData *clients = (clientData *) calloc(n, sizeof(clientData));
    for (int i = 0; i < n; i++) {
        memset(clients[i].first_name, 0, 13);
        memset(clients[i].last_name, 0, 13);
        fscanf(fd, "%s %s %d %d %s %lf", clients[i].first_name, clients[i].last_name,
               &clients[i].no_card, &clients[i].pin, clients[i].password, &clients[i].sold);
        clients[i].connected = 0;
        clients[i].failed_attempts = 0;
        clients[i].associated_fd = 0;
    }
    *no_clients = n;
    return clients;
}

// determinare tip comanda primita de la client
int get_command(char *buf, char **no_card, char **pin_password_sum) {
    char *buf_copy = strdup(buf);
    char *token = strtok(buf_copy, " \n");
    *no_card = NULL;
    *pin_password_sum = NULL;
    if (strcmp(token, "login") == 0) {
        *no_card = strtok(NULL, " ");
        *pin_password_sum = strtok(NULL, " ");
        return LOGIN;
    }
    if (strcmp(token, "unlock") == 0) {
        *no_card = strtok(NULL, " ");
        return UNLOCK;
    }

    if (strcmp(token, "quit") == 0) {
        return QUIT;
    }

    if (strcmp(token, "logout") == 0) {
        return LOGOUT;
    }

    if (strcmp(token, "listsold") == 0) {
        return LISTSOLD;
    }

    if (strcmp(token, "transfer") == 0) {
        *no_card = strtok(NULL, " ");
        *pin_password_sum = strtok(NULL, " \n");
        return TRANSFER;
    }
    // parola de unlock (numar card + parola)
    if (strtol(token, 0, 10)) {
        *no_card = token;
        *pin_password_sum = strtok(NULL, " ");
        return UNLOCK;
    }
    return UNKNOWN_COMMAND;
}

// verificare existenta client cu numarul de card no_card
int is_client_in_data_base(clientData *clientVector, int len, int no_card) {
    for (int i = 0; i < len; i++) {
        if (clientVector[i].no_card == no_card) {
            return i;
        }
    }
    return -1;
}

// mapare client fizic <-> socket
int search_associated_client(clientData *clientVector, int len, int fd) {
    for (int i = 0; i < len; i++) {
        if (clientVector[i].associated_fd == fd) {
            return i;
        }
    }
    return -1;
}

// verificare cod pin
int verify_pin_code(char *buffer, clientData *client, int client_fd, int pin) {
    // cod pin bun si cardul nu e blocat
    if (client->failed_attempts < 3 && client->pin == pin) {
        client->connected = CONNECTED;
        client->failed_attempts = 0;                          //resetare incercari esuate
        client->associated_fd = client_fd;
        memset(buffer, 0, strlen(buffer));
        strcpy(buffer, "Welcome ");
        strcpy(buffer + 8, client->first_name);
        strcpy(buffer + 8 + strlen(client->first_name), " ");
        strcpy(buffer + 8 + strlen(client->first_name) + 1, client->last_name);
        send(client_fd, buffer, strlen(buffer) + 1, 0);
        return 0;
        // codul pin nu e corect
    } else {
        client->failed_attempts++;
        if (client->failed_attempts >= 3) {
            memset(buffer, 0, strlen(buffer));
            strcpy(buffer, "-5");                            //pin gresit de trei ori
            send(client_fd, buffer, strlen(buffer) + 1, 0);
            return 1;
        }
        memset(buffer, 0, strlen(buffer));
        strcpy(buffer, "-3");                               //pin gresit
        send(client_fd, buffer, strlen(buffer) + 1, 0);
        return 1;
    }
}

// verificare client deja conectat
int verify_already_connected_client(char *buffer, clientData client, int client_fd) {
    if (client.connected == CONNECTED) {
        memset(buffer, 0, strlen(buffer));
        strcpy(buffer, "-2");                               //client deja conectat
        send(client_fd, buffer, strlen(buffer) + 1, 0);
        return CONNECTED;
    }
    return UNCONNECTED;
}

// deblocare card
char *unlock_card(char *buffer, clientData *client) {
    char *buf_copy = strdup(buffer);
    if (client->failed_attempts < 3) {     //cardul nu e blocat
        memset(buffer, 0, BUFLEN);
        strcpy(buffer, "-6");
        return buffer;
    }
    if (strcmp(strtok(buf_copy, " "), "unlock") == 0) {
        memset(buffer, 0, BUFLEN);
        strcpy(buffer, "Trimite");
        return buffer;
    }
    char *token = strtok(NULL, " \n");
    if (strcmp(token, client->password) != 0) {
        printf("%s ", client->password);
        memset(buffer, 0, BUFLEN);
        strcpy(buffer, "-7");
        return buffer;
    } else {
        memset(buffer, 0, BUFLEN);
        strcpy(buffer, "Card deblocat");
        client->failed_attempts = 0;
        return buffer;
    }
}

#endif //TEMA2_SERVER_H
