
#include "../common/socket_utils.h"
#include "../common/http_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>


typedef struct {
    int socket_cliente;
    struct sockaddr_storage client_addr_storage; 
} thread_data_t;

void *thread_tratar_conexao(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    int socket_cli = data->socket_cliente;
    struct sockaddr_storage client_addr = data->client_addr_storage; 

    
    
    
    
    free(data);


    char *ip_cliente_str = obter_ip_cliente(&client_addr);
    printf("Thread (TID: %lu) tratando cliente %s\n", (unsigned long)pthread_self(), ip_cliente_str ? ip_cliente_str : "IP desconhecido");
    if(ip_cliente_str) free(ip_cliente_str);

    tratar_conexao_http(socket_cli, &client_addr);
    

    pthread_detach(pthread_self()); 
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int porta = atoi(argv[1]);
    int socket_escuta;
    
    
    socklen_t tamanho_endereco_cliente;
    pthread_t tid;

    socket_escuta = criar_socket_escuta(porta);
    if (socket_escuta == -1) {
        exit(EXIT_FAILURE);
    }

    printf("Servidor Thread por Cliente iniciado. Aguardando conexões...\n");

    while (1) {
        
        thread_data_t *data = (thread_data_t *)malloc(sizeof(thread_data_t));
        if (!data) {
            perror("Erro ao alocar memória para dados da thread");
            continue; 
        }

        tamanho_endereco_cliente = sizeof(data->client_addr_storage);
        data->socket_cliente = accept(socket_escuta, (struct sockaddr *)&(data->client_addr_storage), &tamanho_endereco_cliente);
        
        if (data->socket_cliente < 0) {
            perror("Erro no accept");
            free(data); 
            continue;
        }

        char *ip_cliente_str_main = obter_ip_cliente(&(data->client_addr_storage));
        printf("Conexão aceita de %s na thread principal.\n", ip_cliente_str_main ? ip_cliente_str_main : "IP desconhecido");
        if(ip_cliente_str_main) free(ip_cliente_str_main);

        if (pthread_create(&tid, NULL, thread_tratar_conexao, (void *)data) != 0) {
            perror("Erro ao criar thread");
            close(data->socket_cliente); 
            free(data); 
        }
        
        
                             
    }

    close(socket_escuta);
    return 0;
}
