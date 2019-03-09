//
// Created by mihaela on 02.05.2018.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>


#ifndef TEMA2_LIB_H
#define TEMA2_LIB_H

// MACRO -uri folosite la verificare si afisare
#define UNKNOWN_COMMAND 0
#define LOGIN 1
#define UNLOCK 2
#define QUIT 3
#define LOGOUT 4
#define LISTSOLD 5
#define TRANSFER 6
#define CONNECTED 1
#define UNCONNECTED 0
#define STDIN 0
#define TRUE 1;
#define FALSE 0

#endif //TEMA2_LIB_H
