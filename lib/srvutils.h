//
// Created by darkwolf on 15/07/17.
//

#ifndef TP1_SRVUTILS_H
#define TP1_SRVUTILS_H

#include <netinet/in.h>


#define MSG_BUFFER_SIZE 256     // Msg size
#define SLIST_ADDR_MAX_SIZE 25  // Format "IP:Port" size
#define SECURITY_MARGIN_SIZE 10   // Security margin to avoid segmentation fault


// Log format:  "[IP:Port]: Msg" total size (for server receiving msg)
//              ":~$ IP:Port Msg" total size (for server sending msg)
#define LOG_BUFFER_SIZE SLIST_ADDR_MAX_SIZE + MSG_BUFFER_SIZE + SECURITY_MARGIN_SIZE


// Commands (server
#define SEND_MSG_CMD ":~$ " // Usage: :~$ [IP:Port] [Msg]


char* addr_wrapper(char (*addr)[SLIST_ADDR_MAX_SIZE], struct sockaddr_in addr_in);

char* log_wrapper(char (*log)[LOG_BUFFER_SIZE], char *addr, char *message);

char* client_logon_message(char (*log)[LOG_BUFFER_SIZE], char *addr);

char* client_logout_message(char (*log)[LOG_BUFFER_SIZE], char *addr);


#endif //TP1_SRVUTILS_H
