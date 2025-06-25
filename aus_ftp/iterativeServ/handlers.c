// handlers.c

#include "responses.h"
#include "pi.h"
#include "dtp.h"
#include "session.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

void handle_USER(const char *args) {
  ftp_session_t *sess = session_get();

  if (!args || strlen(args) == 0) {
    safe_dprintf(sess->control_sock, MSG_501); // Syntax error in parameters
    return;
  }

  strncpy(sess->current_user, args, sizeof(sess->current_user) - 1);
  sess->current_user[sizeof(sess->current_user) - 1] = '\0';
  safe_dprintf(sess->control_sock, MSG_331); // Username okay, need password
}

void handle_PASS(const char *args) {

  ftp_session_t *sess = session_get();

  if (sess->current_user[0] == '\0') {
    safe_dprintf(sess->control_sock, MSG_503); // Bad sequence of commands
    return;
  }

  if (!args || strlen(args) == 0) {
    safe_dprintf(sess->control_sock, MSG_501); // Syntax error in parameters
    return;
  }

  if (check_credentials(sess->current_user, (char *)args) == 0) {
    sess->logged_in = 1;
    safe_dprintf(sess->control_sock, MSG_230); // User logged in
  } else {
    safe_dprintf(sess->control_sock, MSG_530); // Not logged in
    sess->current_user[0] = '\0'; // Reset user on failed login
    sess->logged_in = 0;
  }
}

void handle_QUIT(const char *args) {
  ftp_session_t *sess = session_get();
  (void)args; // unused

  safe_dprintf(sess->control_sock, MSG_221); // 221 Goodbye.
  sess->current_user[0] = '\0'; // Close session
  close_fd(sess->control_sock, "client socket"); // Close socket
  sess->control_sock = -1;
}

void handle_SYST(const char *args) {
  ftp_session_t *sess = session_get();
  (void)args; // unused

  safe_dprintf(sess->control_sock, MSG_215); // 215 <system type>
}

void handle_TYPE(const char *args) {
  ftp_session_t *sess = session_get();
  (void)args;
  (void)sess;

  if (!args || strlen(args) == 0) {
    safe_dprintf(sess->control_sock, MSG_501); // Syntax error in parameters
    return;
  }

  if(args[0] != 'I'){
    safe_dprintf(sess->control_sock, MSG_504); // Command not implemented for that parameter
    return;
  }

  safe_dprintf(sess->control_sock, MSG_200); // Command okey

} // Finish TYPE

void handle_PORT(const char *args) {
  ftp_session_t *sess = session_get();
  (void)args;
  (void)sess;

  if (!args || strlen(args) == 0) {
    safe_dprintf(sess->control_sock, MSG_501); // Syntax error in parameters
    return;
  }

  int h1, h2, h3, h4, p1, p2;
  if(sscanf(args, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2) != 6 ||
      (h1 < 0 || h1 > 255) || (h2 < 0 || h2 > 255) ||
      (h3 < 0 || h3 > 255) || (h4 < 0 || h4 > 255) ||
      (p1 < 0 || p1 > 255) || (p2 < 0 || p2 > 255)) 
      {
        safe_dprintf(sess->control_sock, MSG_501); // Syntax error in parameters
        return;
      }

  char ip[INET_ADDRSTRLEN];
  snprintf(ip, sizeof(ip), "%d.%d.%d.%d", h1, h2, h3, h4);

  sess->data_addr.sin_family = AF_INET; //IPv4
  sess->data_addr.sin_port = htons(p1*256 + p2); // Set port

  if(inet_pton(AF_INET, ip, &sess->data_addr.sin_addr) <= 0){ // IP to binary
    safe_dprintf(sess->control_sock, MSG_501);
    return;
  }
  printf("FTP client ask for active connection at %s:%d\n", ip, ntohs(sess->data_addr.sin_port));
  safe_dprintf(sess->control_sock, MSG_200); // Command okey


} // Finish PORT

void handle_RETR(const char *args) {
  ftp_session_t *sess = session_get();
  (void)args;
  (void)sess;

  if (!args || strlen(args) == 0) {
    safe_dprintf(sess->control_sock, MSG_501); // Error in parameters
    return;
  }

  if(sess->data_addr.sin_port == 0){
    safe_dprintf(sess->control_sock, MSG_503); // Need PORT first
    return;
  }
  
  
  FILE *file = fopen(args, "rb");
  if(!file){
    safe_dprintf(sess->control_sock, MSG_550, args); // File unavailable
    return;
  }
  safe_dprintf(sess->control_sock, MSG_150); // File status okay, about to open data connection
  
  fseek(file, 0, SEEK_END);
  long filesize = ftell(file);
  rewind(file);
  safe_dprintf(sess->control_sock, MSG_213, args, filesize); // File status
  safe_dprintf(sess->control_sock, "File %s - Size %ld\r\n", args, filesize);

  int datasock = socket(AF_INET, SOCK_STREAM, 0); // IPv4, TCP
  if(connect(datasock, (struct sockaddr *)&sess->data_addr, sizeof(sess->data_addr)) < 0){
    safe_dprintf(sess->control_sock, MSG_425); // Can't open data connection
    fclose(file);
    return;
  }
  safe_dprintf(sess->control_sock, MSG_125); // Data connection already open
  
  char databuff[BUFFER_SIZE];
  ssize_t bytes;
  while((bytes = fread(databuff, 1, sizeof(databuff), file)) > 0){
    if(send(datasock, databuff, bytes, 0) != bytes){
      perror("Error sending file: ");
      break;
    }
  }

  fclose(file);
  close(datasock);
  safe_dprintf(sess->control_sock, MSG_226);
} // Finish RETR

void handle_STOR(const char *args) {
  ftp_session_t *sess = session_get();
  (void)args;
  (void)sess;

  if (!args || strlen(args) == 0) {
    safe_dprintf(sess->control_sock, MSG_501); // Error in parameters
    return;
  }
  if(sess->data_addr.sin_port == 0){
    safe_dprintf(sess->control_sock, MSG_503); // Need PORT first
    return;
  }

  int datasock = socket(AF_INET, SOCK_STREAM, 0); // IPv4, TCP
  if(connect(datasock, (struct sockaddr *)&sess->data_addr, sizeof(sess->data_addr)) < 0){
    safe_dprintf(sess->control_sock, MSG_425); // Can't open data connection
    return;
  }
  safe_dprintf(sess->control_sock, MSG_125); // Data connection already open

  
  FILE *file = fopen(args, "wb");
  if(!file){
    safe_dprintf(sess->control_sock, MSG_550, args); // File unavailable
    return;
  }
  
  char databuff[BUFFER_SIZE];
  ssize_t bytes;
  while((bytes = recv(datasock, databuff, sizeof(databuff), 0)) > 0){
    fwrite(databuff, 1, bytes, file);
  }

  fclose(file);
  close(datasock);

  safe_dprintf(sess->control_sock, MSG_226);
} // Finish STOR

void handle_NOOP(const char *args) {
  ftp_session_t *sess = session_get();
  (void)args;
  //(void)sess;
  safe_dprintf(sess->control_sock, MSG_200); // Command okey
}
