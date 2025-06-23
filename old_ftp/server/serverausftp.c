#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "serverdtp.h"
#include "serverpi.h"
#include "serverausftp.h"

#define DEFAULT_PORT 21




int main(int argc, char const *argv[]){

    int port;

    if(argc > 2){
        fprintf(stderr, "Error: mal ingreso de argumentos\n");
        return 1;

    }
    if(argc == 2){
        port = atoi(argv[1]);

    }else{
        port = DEFAULT_PORT;
    }
    
    if(port == 0 || port > 65535){
        fprintf(stderr, "Error: puerto invalido \n");
        return 1;
    }

    int mastersocket, slavesocket;
    struct sockaddr_in masteraddr, slaveaddr;
    socklen_t slaveaddrlen;
    char user_name[BUFFSIZE];
    char user_pass[BUFFSIZE];
    char buffer[BUFFSIZE];
    char command[BUFFSIZE];
    int data_len;

    mastersocket = socket(AF_INET, SOCK_STREAM, 0);
    masteraddr.sin_family = AF_INET;
    masteraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    masteraddr.sin_port = htons(port);


    bind(mastersocket, (struct sockaddr *) &masteraddr, sizeof(masteraddr));
    listen(mastersocket, 5);

    while(true){

        slaveaddrlen = sizeof(slaveaddr);
        slavesocket = accept(mastersocket, (struct sockaddr *) &slaveaddr, &slaveaddrlen);
        //printf("%s\n", MSG_220);

        if (send(slavesocket, MSG_220, sizeof(MSG_220) - 1, 0) != sizeof(MSG_220)- 1){ 
            close(slavesocket);
            fprintf(stderr, "Error: no se pudo enviar el mensaje.\n");
            break;
        }

        if(recv_cmd(slavesocket, command, user_name) != 0 ){
            close(slavesocket);
            fprintf(stderr, "Error: no se pudo recibir el comanod USER.\n");
            break;
        }

        if(strcmp(command, "USER") != 0){
            close(slavesocket);
            fprintf(stderr, "Error: se esperaba el comando USER.\n");
            continue;
        }

        data_len = snprintf(buffer, BUFFSIZE, MSG_331, user_name);
        if(send(slavesocket, buffer, data_len, 0) < 0) {
            close(slavesocket);
            fprintf(stderr, "Error: no se pudo enviar el mnensaje MSG_331.\n");
            break;
        }

        if (recv_cmd(slavesocket, command, user_pass) != 0){
            close(slavesocket);
            fprintf(stderr, "Error: no se pudo recibir el comando PASS.\n");
            break;
        } 

        if(strcmp(command, "PASS") != 0){
            close(slavesocket);
            fprintf(stderr, "Error: se esperaba el comando PASS.\n");
            break;
        }

        if(!check_credentials(user_name, user_pass)){
            data_len = snprintf(buffer, BUFFSIZE, MSG_530);
            if(send(slavesocket, buffer, data_len, 0) < 0) {
                close(slavesocket);
                fprintf(stderr, "Error: no se pudo enviar el mnensaje MSG_530.\n");
                break;
            }
            close(slavesocket);
            continue;
        }
        data_len = snprintf(buffer, BUFFSIZE, MSG_230, user_name);
        if(send(slavesocket, buffer, data_len, 0) < 0){
            close(slavesocket);
            fprintf(stderr, "Error: no se pudo enviar el mensaje MSG_230");
            break;
        }
        

        //  BUCLE COMANDOS
        while(true){

            if(recv_cmd(slavesocket, command, buffer) != 0){
                close(slavesocket);
                fprintf(stderr,"Error: no se recibio el comando.\r\n");
                break;
            }
            if(strcmp(command, "QUIT") == 0){                                   // COMANDO QUIT
                if(send(slavesocket, MSG_221, sizeof(MSG_221)-1, 0) < 0){
                    close(slavesocket);
                    fprintf(stderr, "Error: no se pudo enviar el mensaje MSG_221.\r\n");
                    break;
                }
                close(slavesocket);
                break;
            }

            if(strcmp(command, "SYST") == 0){                                   // COMANDO SYST
                if(send(slavesocket, MSG_215, sizeof(MSG_215)-1, 0) < 0){
                    close(slavesocket);
                    fprintf(stderr,"Error: no se pudo enviar el mensaje MSG_215\r\n");
                    break;
                }
                continue;
            }

            if(strcmp(command, "FEAT") == 0){                                   // COMANDO FEAT
                if(send(slavesocket, MSG_502, sizeof(MSG_502)-1, 0) < 0){
                    close(slavesocket);
                    fprintf(stderr, "Erro: no se pudo enviar el mensaje 502\r\n");
                    break;
                }
                continue;
            }
            
             if(strcmp(command, "PORT") == 0){                                   // COMANDO PORT
                int h1, h2, h3, h4, p1, p2;
                if(sscanf(buffer, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2) != 6){
                    if(send(slavesocket, MSG_501, sizeof(MSG_501)-1, 0) < 0){
                        close(slavesocket);
                        fprintf(stderr, "Error: no se pudo enviar el mensjae 501\r\n");
                        break;
                    }
                }
            }

        }//Fin bucle comandos


    }

    close(mastersocket);

    return 0;
}