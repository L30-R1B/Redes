
#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include <netinet/in.h> 

#define WWWROOT "./wwwroot" 
#define BUFFER_SIZE 4096



void tratar_conexao_http(int socket_cliente, struct sockaddr_storage *client_addr);

#endif 