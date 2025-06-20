#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "serverausftp.h"
#include "serverpi.h"

int is_valid_command(const char *command){
    int i = 0;
    while(valid_commands[i] != NULL){
        if(strcmp(command, valid_commands[i]) == 0) {
            return arg_commands[i];
        }
        i++;
    }
    return -1;
}

//COMUNICACION

/* 
    recv_cmd Recepciona un comando desde el socketDescriptor
    recv hace el receive / llamada definida en syssocket
    strtok se comporta distinto a medida que se va llamando / devuelve tokens, 
    la primera vez se le pasas el buffer y el separador p indentificar tokens distintos

    en operation obtengo la operacion-comandos basicos de ftp- grlmente de 4 letras, lo que mando el cliente

*/

int recv_cmd(int socketDescriptor, char *operation, char *param) {
    char buffer[BUFFSIZE];
    char *token;
    int args_number;

    if (recv(socketDescriptor, buffer, BUFFSIZE, 0) < 0) { 
        fprintf(stderr,"Error: no se pudo recibir el comando.\n");
        return 1;
    }

    buffer[strcspn(buffer, "\r\n")] = 0;
    token = strtok(buffer, " ");
    args_number = is_valid_command(token);
    if(token == NULL || strlen(token) < 3 || args_number < 0){
        fprintf(stderr, "Error: comando no válido.\n");
        return 1;
    }

        
    strcpy(operation, token); // Arreglar

            
    if(!args_number)
        return 0;

    
    token = strtok(NULL, " "); // trabaja con el buffer que tenia
    #if DEBUG 
    printf("par %s\n", token); // cuando compilas pones -D DEBUG, se compila con lo que esta entre #if
     #endif
    if (token != NULL) { 
        strcpy(param, token);
    }else{
        fprintf(stderr, "Error: se esperaba un argumento apar el comando %s.\n", operation);
        return 1;
    }

    return 0;
}
        
