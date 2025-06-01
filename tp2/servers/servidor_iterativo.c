
#include "../common/socket_utils.h"
#include "../common/http_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int porta = atoi(argv[1]);
    int socket_escuta, socket_cliente;
    struct sockaddr_storage endereco_cliente; 
    socklen_t tamanho_endereco_cliente;

    socket_escuta = criar_socket_escuta(porta);
    if (socket_escuta == -1) {
        exit(EXIT_FAILURE);
    }

    printf("Servidor Iterativo iniciado. Aguardando conexões...\n");

    while (1) {
        tamanho_endereco_cliente = sizeof(endereco_cliente);
        socket_cliente = accept(socket_escuta, (struct sockaddr *)&endereco_cliente, &tamanho_endereco_cliente);
        
        if (socket_cliente < 0) {
            perror("Erro no accept");
            continue; 
        }
        
        char *ip_cliente_str = obter_ip_cliente(&endereco_cliente);
        printf("Conexão aceita de %s\n", ip_cliente_str ? ip_cliente_str : "IP desconhecido");
        if(ip_cliente_str) free(ip_cliente_str);


        tratar_conexao_http(socket_cliente, &endereco_cliente);
        
    }

    close(socket_escuta); 
    return 0;
}
