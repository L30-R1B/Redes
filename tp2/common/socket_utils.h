
#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include <netinet/in.h> 



int criar_socket_escuta(int porta);



char* obter_ip_cliente(struct sockaddr_storage *addr);

#endif 
