#include "lib.h"

#ifndef TEMA2_CLIENT_H
#define TEMA2_CLIENT_H

// verificare comanda primita
// functia returneaza o valoare corespunzatoare fiecarei comenzi (MACRO-uri def. in lib.h)
int check_command(char *buf, char **lst_login_no_card) {
    char *buf_copy = strdup(buf);
    char *token = strtok(buf_copy, " \n");
    if (strcmp(token, "login") == 0) {
        *lst_login_no_card = strtok(NULL, " ");
        token = strtok(NULL, " ");
        if (*lst_login_no_card == NULL || token == NULL)
            return UNKNOWN_COMMAND;
        return LOGIN;
    }

    if (strcmp(token, "unlock") == 0) {
        return UNLOCK;
    }

    if (strcmp(token, "quit") == 0) {
        return QUIT;
    }
    if (strcmp(token, "logout") == 0) {
        return LOGOUT;
    }
    if (strcmp(token, "listsold") == 0) {
        return LOGOUT;
    }

    if (strcmp(token, "transfer") == 0) {
        *lst_login_no_card = strtok(NULL, " ");
        return TRANSFER;
    }
    return UNKNOWN_COMMAND;
}

// verificare comanda ce necesita un client autentificat
int need_login_before(int code) {
    if (code != UNKNOWN_COMMAND && code != LOGIN && code != UNLOCK && code != QUIT)
        return TRUE;
    return FALSE;
}

#endif //TEMA2_CLIENT_H
