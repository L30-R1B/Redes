
#include "../common/socket_utils.h"
#include "../common/http_handler.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <errno.h>

#define MAX_CLIENTS FD_SETSIZE 





typedef struct {
    int fd;
    struct sockaddr_storage client_addr_storage;
} client_info_t;


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int porta = atoi(argv[1]);
    int socket_escuta, novo_socket;
    
    client_info_t client_infos[MAX_CLIENTS]; 
    int max_clientes = MAX_CLIENTS;
    int atividade, i, valread;
    int max_sd; 
    fd_set readfds; 

    
    for (i = 0; i < max_clientes; i++) {
        client_infos[i].fd = 0;
    }

    socket_escuta = criar_socket_escuta(porta);
    if (socket_escuta == -1) {
        exit(EXIT_FAILURE);
    }

    printf("Servidor Select iniciado. Aguardando conexões...\n");

    while (1) {
        FD_ZERO(&readfds); 

        
        FD_SET(socket_escuta, &readfds);
        max_sd = socket_escuta;

        
        for (i = 0; i < max_clientes; i++) {
            int sd = client_infos[i].fd;
            if (sd > 0) { 
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) { 
                max_sd = sd;
            }
        }

        
        atividade = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((atividade < 0) && (errno != EINTR)) {
            perror("Erro no select");
            
        }

        
        if (FD_ISSET(socket_escuta, &readfds)) {
            struct sockaddr_storage endereco_cliente_temp;
            socklen_t tamanho_endereco_cliente_temp = sizeof(endereco_cliente_temp);
            
            novo_socket = accept(socket_escuta, (struct sockaddr *)&endereco_cliente_temp, &tamanho_endereco_cliente_temp);
            if (novo_socket < 0) {
                perror("Erro no accept");
            } else {
                char *ip_cliente_str = obter_ip_cliente(&endereco_cliente_temp);
                printf("Nova conexão de %s, socket fd é %d\n", ip_cliente_str ? ip_cliente_str : "IP desc.", novo_socket);
                if(ip_cliente_str) free(ip_cliente_str);

                
                for (i = 0; i < max_clientes; i++) {
                    if (client_infos[i].fd == 0) { 
                        client_infos[i].fd = novo_socket;
                        client_infos[i].client_addr_storage = endereco_cliente_temp; 
                        printf("Adicionando à lista de sockets como %d\n", i);
                        break;
                    }
                }
                 if (i == max_clientes) { 
                    fprintf(stderr, "Muitos clientes conectados. Rejeitando nova conexão.\n");
                    close(novo_socket);
                }
            }
        }

        
        for (i = 0; i < max_clientes; i++) {
            int sd = client_infos[i].fd;

            if (FD_ISSET(sd, &readfds)) {
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                char *ip_cliente_str_handler = obter_ip_cliente(&(client_infos[i].client_addr_storage));
                printf("Tratando dados do cliente %s (socket %d)\n", ip_cliente_str_handler ? ip_cliente_str_handler : "IP desc.", sd);
                if(ip_cliente_str_handler) free(ip_cliente_str_handler);

                
                
                
                
                
                
                
                tratar_conexao_http(sd, &(client_infos[i].client_addr_storage));
                
                
                
                FD_CLR(sd, &readfds); 
                client_infos[i].fd = 0; 
            }
        }
    }

    close(socket_escuta);
    return 0;
}
