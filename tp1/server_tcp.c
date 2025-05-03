#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <openssl/evp.h>

#define BUFFER_SIZE 4096
#define TIME_LOG_FILE "transfer_times.log"

void flush_socket(int socket) {
    char buffer[BUFFER_SIZE];
    struct timeval tv;
    fd_set fds;
    
    tv.tv_sec = 0;
    tv.tv_usec = 10000;
    FD_ZERO(&fds);
    FD_SET(socket, &fds);
    
    while(select(socket+1, &fds, NULL, NULL, &tv) > 0) {
        if(recv(socket, buffer, BUFFER_SIZE, 0) <= 0) break;
        tv.tv_sec = 0;
        tv.tv_usec = 10000;
        FD_ZERO(&fds);
        FD_SET(socket, &fds);
    }
}

void reliable_send(int socket, const void *buffer, size_t length) {
    size_t total_sent = 0;
    while(total_sent < length) {
        ssize_t sent = send(socket, (char*)buffer + total_sent, length - total_sent, 0);
        if(sent <= 0) {
            perror("Erro ao enviar dados");
            return;
        }
        total_sent += sent;
    }
}

void send_file(int client_socket, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if(file == NULL) {
        perror("Erro ao abrir arquivo");
        reliable_send(client_socket, "ERROR", 6);
        return;
    }

    // Limpa qualquer dado residual
    flush_socket(client_socket);

    // Envia confirmação
    reliable_send(client_socket, "OK", 3);

    // Obtém e envia tamanho do arquivo
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char size_str[20];
    snprintf(size_str, sizeof(size_str), "%ld", file_size);
    reliable_send(client_socket, size_str, strlen(size_str)+1);

    // Configura MD5
    EVP_MD_CTX *md5_context = EVP_MD_CTX_new();
    if(!md5_context || EVP_DigestInit_ex(md5_context, EVP_md5(), NULL) != 1) {
        perror("Erro ao configurar MD5");
        fclose(file);
        return;
    }

    // Envia conteúdo do arquivo
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    struct timeval start, end;
    gettimeofday(&start, NULL);

    while((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        reliable_send(client_socket, buffer, bytes_read);
        EVP_DigestUpdate(md5_context, buffer, bytes_read);
    }

    gettimeofday(&end, NULL);
    double time_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
    
    // Log do tempo
    FILE *log_file = fopen(TIME_LOG_FILE, "a");
    if(log_file) {
        fprintf(log_file, "Arquivo: %s - Tempo: %.2f ms\n", filename, time_ms);
        fclose(log_file);
    }

    // Envia hash MD5
    unsigned char md5_digest[EVP_MAX_MD_SIZE];
    unsigned int md5_len;
    EVP_DigestFinal_ex(md5_context, md5_digest, &md5_len);
    reliable_send(client_socket, md5_digest, md5_len);

    EVP_MD_CTX_free(md5_context);
    fclose(file);
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <porta> <diretorio_arquivos>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    const char *file_dir = argv[2];

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao fazer bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Erro ao ouvir");
        exit(EXIT_FAILURE);
    }

    printf("Servidor TCP ouvindo na porta %d...\n", port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Erro ao aceitar conexão");
            continue;
        }

        printf("Cliente conectado: %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        char command[256];
        while (1) {
            memset(command, 0, sizeof(command));
            int recv_len = recv(client_socket, command, sizeof(command), 0);
            if (recv_len <= 0) {
                printf("Cliente desconectado ou erro na conexão\n");
                break;
            }

            if (strncmp(command, "exit", 4) == 0) {
                printf("Recebido comando 'exit', encerrando conexão com cliente\n");
                break;
            }

            if (strncmp(command, "get ", 4) == 0) {
                char filename[256];
                strcpy(filename, file_dir);
                strcat(filename, "/");
                strcat(filename, command + 4);
                printf("Solicitado arquivo: %s\n", filename);
                send_file(client_socket, filename);
            } else {
                printf("Comando desconhecido: %s\n", command);
                send(client_socket, "Comando desconhecido", 20, 0);
            }
        }

        close(client_socket);
    }

    close(server_socket);
    return 0;
}