#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "serverausftp.h"

#define MSG_220 "220 srvFtp version 1.0\r\n"
#define MSG_331 "331 Password required for %s\r\n"
#define MSG_230 "230 User %s logged in\r\n"
#define MSG_530 "530 Login incorrect\r\n"
#define MSG_221 "221 Goodbye\r\n"
#define MSG_550 "550 %s: no such file or directory\r\n"
#define MSG_299 "299 File %s size %ld bytes\r\n"
#define MSG_226 "226 Transfer complete\r\n"

//COMUNICACION

/* 
    recv_cmd Recepciona un comando desde el socketDescriptor
    recv hace el receive / llamada definida en syssocket
    strtok se comporta distinto a medida que se va llamando / devuelve tokens, 
    la primera vez se le pasas el buffer y el separador p indentificar tokens distintos

    en operation obtengo la operacion-comandos basicos de ftp- grlmente de 4 letras, lo que mando el cliente

*/

int recv_cmd(int socketDescriptor, char *operation, char *param) {
    char buffer[BUFSIZE];
    char *token;

    if (recv(socketDescriptor, buffer, BUFSIZE, 0) < 0) { 
        fprintf(stderr,"error receiving data");
        return 1;
    }

    buffer[strcspn(buffer, "\r\n")] = 0;
    token = strtok(buffer, " ");
    if (token == NULL || strlen(token) < 4) {
        fprintf(stderr,"not valid ftp command");
        return 1;
    } else {
        strcpy(operation, token);
        token = strtok(NULL, " "); // trabaja con el buffer que tenia
        #if DEBUG 
        printf("par %s\n", token); // cuando compilas pones -D DEBUG, se compila con lo que esta entre #if
        #endif
        if (token != NULL) strcpy(param, token);
    }
}