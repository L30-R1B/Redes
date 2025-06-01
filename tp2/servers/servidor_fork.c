
#include "../common/socket_utils.h"
#include "../common/http_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h> 
#include <errno.h>    
#include <signal.h>   

void sigchld_handler(int s) {
    (void)s; 
    
    
    int saved_errno = errno; 
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int porta = atoi(argv[1]);
    int socket_escuta, socket_cliente;
    struct sockaddr_storage endereco_cliente;
    socklen_t tamanho_endereco_cliente;
    struct sigaction sa;

    socket_escuta = criar_socket_escuta(porta);
    if (socket_escuta == -1) {
        exit(EXIT_FAILURE);
    }

    
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; 
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("Erro ao configurar sigaction para SIGCHLD");
        exit(EXIT_FAILURE);
    }

    printf("Servidor Fork iniciado. Aguardando conexões...\n");

    while (1) {
        tamanho_endereco_cliente = sizeof(endereco_cliente);
        socket_cliente = accept(socket_escuta, (struct sockaddr *)&endereco_cliente, &tamanho_endereco_cliente);
        
        if (socket_cliente < 0) {
            perror("Erro no accept");
            continue;
        }
        
        char *ip_cliente_str_pai = obter_ip_cliente(&endereco_cliente);
        printf("Conexão aceita de %s no processo pai.\n", ip_cliente_str_pai ? ip_cliente_str_pai : "IP desconhecido");
        if(ip_cliente_str_pai) free(ip_cliente_str_pai);

        pid_t pid = fork();
        if (pid < 0) {
            perror("Erro no fork");
            close(socket_cliente); 
            continue;
        } else if (pid == 0) { 
            close(socket_escuta); 

            char *ip_cliente_str_filho = obter_ip_cliente(&endereco_cliente);
             printf("Processo filho (PID: %d) tratando cliente %s\n", getpid(), ip_cliente_str_filho ? ip_cliente_str_filho : "IP desconhecido");
            if(ip_cliente_str_filho) free(ip_cliente_str_filho);

            tratar_conexao_http(socket_cliente, &endereco_cliente);
            
            exit(EXIT_SUCCESS); 
        } else { 
            close(socket_cliente); 
        }
    }

    close(socket_escuta);
    return 0;
}
