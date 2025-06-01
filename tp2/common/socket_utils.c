
#include "socket_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <netdb.h>     

int criar_socket_escuta(int porta) {
    int sockfd;
    struct sockaddr_in serv_addr;
    int optval = 1;

    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        return -1;
    }

    
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("Erro ao configurar SO_REUSEADDR");
        close(sockfd);
        return -1;
    }

    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serv_addr.sin_port = htons(porta);

    
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Erro no bind");
        close(sockfd);
        return -1;
    }

    
    if (listen(sockfd, 10) < 0) { 
        perror("Erro no listen");
        close(sockfd);
        return -1;
    }

    printf("Servidor escutando na porta %d...\n", porta);
    return sockfd;
}

char* obter_ip_cliente(struct sockaddr_storage *addr) {
    static char ip_str[INET6_ADDRSTRLEN]; 

    if (addr->ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)addr;
        inet_ntop(AF_INET, &s->sin_addr, ip_str, sizeof(ip_str));
    } else { 
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)addr;
        inet_ntop(AF_INET6, &s->sin6_addr, ip_str, sizeof(ip_str));
    }
    
    
    char *dyn_ip_str = malloc(INET6_ADDRSTRLEN);
    if (dyn_ip_str) {
        strncpy(dyn_ip_str, ip_str, INET6_ADDRSTRLEN);
        dyn_ip_str[INET6_ADDRSTRLEN - 1] = '\0'; 
    }
    return dyn_ip_str;
}


