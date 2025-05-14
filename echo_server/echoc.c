#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define BUFFSIZE 512

int main(){

    int sh, len;
    struct sockaddr_in addr;
    char msgsend[] = "Hola Mundo\n\r";
    char msgrecv[BUFFSIZE];

// Esto va segundo
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(3333);


// Esto va primero
    if((sh = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        printf("Failed to create socket\n");
        exit(1);
    }

    if(connect(sh, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        printf("Failed to connect echo server\n");
        close(sh);
        exit(1);
    }
    
    if(send(sh, msgsend, sizeof(msgsend), 0) != sizeof(msgsend)){
        printf("Failed to send %s\n", msgsend);
        close(sh);
        exit(1);
    }

    if((len = recv(sh, msgrecv, BUFFSIZE, 0)) < 0) {
        printf("Failed to receive echo\n");
        close(sh);
        exit(1);
    }
 
    printf("Echo %s", msgrecv);
    close(sh);

    exit(0);
}
