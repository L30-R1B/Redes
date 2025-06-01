

#include "http_handler.h"
#include "logger.h"
#include "mime_types.h"
#include "socket_utils.h" 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h> 
#include <fcntl.h>    
#include <arpa/inet.h> 

#define MAX_PATH_LEN 2048

void enviar_resposta_erro(int socket_cliente, int status_code, const char *status_message, const char *body) {
    char header_buffer[BUFFER_SIZE];
    int body_len = body ? strlen(body) : 0;

    sprintf(header_buffer, "HTTP/1.1 %d %s\r\n", status_code, status_message);
    sprintf(header_buffer + strlen(header_buffer), "Content-Type: text/html\r\n");
    sprintf(header_buffer + strlen(header_buffer), "Content-Length: %d\r\n", body_len);
    sprintf(header_buffer + strlen(header_buffer), "Connection: close\r\n");
    sprintf(header_buffer + strlen(header_buffer), "\r\n");

    send(socket_cliente, header_buffer, strlen(header_buffer), 0);
    if (body) {
        send(socket_cliente, body, body_len, 0);
    }
}

void tratar_conexao_http(int socket_cliente, struct sockaddr_storage *client_addr_storage) {
    char buffer_requisicao[BUFFER_SIZE];
    char metodo[16], uri[MAX_PATH_LEN], versao_http[16];
    char caminho_arquivo_completo[MAX_PATH_LEN + sizeof(WWWROOT)];
    char *ip_cliente_str = NULL;

    
    ip_cliente_str = obter_ip_cliente(client_addr_storage);
    if (!ip_cliente_str) {
        perror("Erro ao obter IP do cliente");
        
        
        ip_cliente_str = strdup("UNKNOWN_IP"); 
    }


    
    ssize_t bytes_lidos = recv(socket_cliente, buffer_requisicao, BUFFER_SIZE - 1, 0);
    if (bytes_lidos <= 0) {
        if (bytes_lidos < 0) perror("Erro ao ler do socket");
        else printf("Cliente fechou a conexão prematuramente.\n");
        close(socket_cliente);
        free(ip_cliente_str);
        return;
    }
    buffer_requisicao[bytes_lidos] = '\0'; 

    
    char *primeira_linha_req = strtok(strdup(buffer_requisicao), "\r\n"); 
    if (primeira_linha_req == NULL) {
        enviar_resposta_erro(socket_cliente, 400, "Bad Request", "<h1>400 Bad Request</h1><p>Requisição mal formatada.</p>");
        close(socket_cliente);
        free(ip_cliente_str);
        return;
    }
    log_request(ip_cliente_str, primeira_linha_req);
    
    
                                  
                                  
                                  
                                  

    char request_line_for_log[BUFFER_SIZE];
    strncpy(request_line_for_log, buffer_requisicao, BUFFER_SIZE);
    char* newline_char = strstr(request_line_for_log, "\r\n");
    if (newline_char) {
        *newline_char = '\0';
    } else {
        newline_char = strchr(request_line_for_log, '\n');
        if (newline_char) *newline_char = '\0';
    }
    

    
    
    if (sscanf(buffer_requisicao, "%15s %2047s %15s", metodo, uri, versao_http) != 3) {
        enviar_resposta_erro(socket_cliente, 400, "Bad Request", "<h1>400 Bad Request</h1><p>Formato da requisição inválido.</p>");
        close(socket_cliente);
        free(ip_cliente_str);
        if (primeira_linha_req) free(primeira_linha_req);
        return;
    }
    
    if (primeira_linha_req) free(primeira_linha_req); 

    
    if (strcmp(metodo, "GET") != 0) {
        enviar_resposta_erro(socket_cliente, 501, "Not Implemented", "<h1>501 Not Implemented</h1><p>Método não suportado.</p>");
        close(socket_cliente);
        free(ip_cliente_str);
        return;
    }

    
    if (strcmp(uri, "/") == 0) {
        strcpy(uri, "/index.html");
    }

    
    snprintf(caminho_arquivo_completo, sizeof(caminho_arquivo_completo), "%s%s", WWWROOT, uri);

    
    if (strstr(caminho_arquivo_completo, "..") != NULL) {
        enviar_resposta_erro(socket_cliente, 400, "Bad Request", "<h1>400 Bad Request</h1><p>Caminho inválido.</p>");
        close(socket_cliente);
        free(ip_cliente_str);
        return;
    }
    
    
    int fd_arquivo = open(caminho_arquivo_completo, O_RDONLY);
    if (fd_arquivo < 0) {
        
        char caminho_404[MAX_PATH_LEN + sizeof(WWWROOT)];
        snprintf(caminho_404, sizeof(caminho_404), "%s/404.html", WWWROOT);
        int fd_404 = open(caminho_404, O_RDONLY);
        if (fd_404 >= 0) {
            struct stat stat_404;
            if (fstat(fd_404, &stat_404) == 0) {
                char header_buffer[BUFFER_SIZE];
                sprintf(header_buffer, "HTTP/1.1 404 Not Found\r\n");
                sprintf(header_buffer + strlen(header_buffer), "Content-Type: text/html\r\n");
                sprintf(header_buffer + strlen(header_buffer), "Content-Length: %ld\r\n", stat_404.st_size);
                sprintf(header_buffer + strlen(header_buffer), "Connection: close\r\n\r\n");
                send(socket_cliente, header_buffer, strlen(header_buffer), 0);

                char buffer_arquivo[BUFFER_SIZE];
                ssize_t bytes_lidos_arquivo;
                while ((bytes_lidos_arquivo = read(fd_404, buffer_arquivo, BUFFER_SIZE)) > 0) {
                    send(socket_cliente, buffer_arquivo, bytes_lidos_arquivo, 0);
                }
            }
            close(fd_404);
        } else {
            enviar_resposta_erro(socket_cliente, 404, "Not Found", "<h1>404 Not Found</h1><p>O recurso solicitado não foi encontrado.</p>");
        }
        close(socket_cliente);
        free(ip_cliente_str);
        return;
    }

    
    struct stat stat_arquivo;
    if (fstat(fd_arquivo, &stat_arquivo) < 0 || !S_ISREG(stat_arquivo.st_mode)) {
        perror("Erro ao obter stat do arquivo ou não é um arquivo regular");
        enviar_resposta_erro(socket_cliente, 500, "Internal Server Error", "<h1>500 Internal Server Error</h1>");
        close(fd_arquivo);
        close(socket_cliente);
        free(ip_cliente_str);
        return;
    }

    long tamanho_arquivo = stat_arquivo.st_size;
    const char *tipo_mime = obter_tipo_mime(caminho_arquivo_completo);

    
    char header_buffer[BUFFER_SIZE];
    sprintf(header_buffer, "HTTP/1.1 200 OK\r\n");
    sprintf(header_buffer + strlen(header_buffer), "Content-Type: %s\r\n", tipo_mime);
    sprintf(header_buffer + strlen(header_buffer), "Content-Length: %ld\r\n", tamanho_arquivo);
    sprintf(header_buffer + strlen(header_buffer), "Connection: close\r\n"); 
    sprintf(header_buffer + strlen(header_buffer), "\r\n"); 

    if (send(socket_cliente, header_buffer, strlen(header_buffer), 0) < 0) {
        perror("Erro ao enviar cabeçalhos HTTP");
        close(fd_arquivo);
        close(socket_cliente);
        free(ip_cliente_str);
        return;
    }

    
    char buffer_arquivo[BUFFER_SIZE];
    ssize_t bytes_lidos_arquivo;
    while ((bytes_lidos_arquivo = read(fd_arquivo, buffer_arquivo, BUFFER_SIZE)) > 0) {
        if (send(socket_cliente, buffer_arquivo, bytes_lidos_arquivo, 0) < 0) {
            perror("Erro ao enviar corpo do arquivo");
            
            break; 
        }
    }
    if (bytes_lidos_arquivo < 0) {
        perror("Erro ao ler do arquivo durante o envio");
    }

    
    close(fd_arquivo);
    close(socket_cliente);
    free(ip_cliente_str);
    
}
