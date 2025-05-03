#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <openssl/evp.h>

#define BUFFER_SIZE 4096
#define SIZE_BUFFER 20

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

int reliable_recv(int socket, void *buffer, size_t length) {
    size_t total_received = 0;
    while(total_received < length) {
        ssize_t received = recv(socket, (char*)buffer + total_received, length - total_received, 0);
        if(received <= 0) {
            return -1;
        }
        total_received += received;
    }
    return 0;
}

void receive_file(int socket, const char *filename) {
    // Limpa socket antes de começar
    flush_socket(socket);

    // Recebe resposta inicial
    char response[10] = {0};
    if(reliable_recv(socket, response, 3) < 0) {
        printf("Erro na comunicação com o servidor\n");
        return;
    }

    if(strcmp(response, "ERROR") == 0) {
        printf("Erro: Arquivo não encontrado no servidor\n");
        return;
    }

    // Recebe tamanho do arquivo
    char size_str[SIZE_BUFFER] = {0};
    char *ptr = size_str;
    int remaining = sizeof(size_str)-1;
    
    while(remaining > 0) {
        ssize_t received = recv(socket, ptr, 1, 0);
        if(received <= 0) {
            printf("Erro ao receber tamanho do arquivo\n");
            return;
        }
        if(*ptr == '\0') break;
        ptr++;
        remaining--;
    }
    
    long file_size = atol(size_str);
    if(file_size <= 0) {
        printf("Erro: Tamanho de arquivo inválido (%ld bytes)\n", file_size);
        return;
    }
    
    printf("Tamanho do arquivo: %ld bytes\n", file_size);

    // Prepara para receber arquivo
    FILE *file = fopen(filename, "wb");
    if(!file) {
        perror("Erro ao criar arquivo local");
        return;
    }

    // Configura MD5
    EVP_MD_CTX *md5_context = EVP_MD_CTX_new();
    if(!md5_context || EVP_DigestInit_ex(md5_context, EVP_md5(), NULL) != 1) {
        perror("Erro ao configurar MD5");
        fclose(file);
        return;
    }

    // Recebe conteúdo do arquivo
    char buffer[BUFFER_SIZE];
    long total_bytes_received = 0;
    struct timeval start, end;
    gettimeofday(&start, NULL);

    while(total_bytes_received < file_size) {
        size_t to_receive = (file_size - total_bytes_received) < BUFFER_SIZE ? 
                          (file_size - total_bytes_received) : BUFFER_SIZE;
        ssize_t received = recv(socket, buffer, to_receive, 0);
        if(received <= 0) {
            perror("Erro ao receber dados");
            break;
        }
        fwrite(buffer, 1, received, file);
        EVP_DigestUpdate(md5_context, buffer, received);
        total_bytes_received += received;
    }

    gettimeofday(&end, NULL);
    double time_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
    printf("Transferência concluída em %.2f ms\n", time_ms);

    // Verifica MD5
    unsigned char server_md5[EVP_MAX_MD_SIZE];
    unsigned char local_md5[EVP_MAX_MD_SIZE];
    unsigned int md5_len;
    
    if(reliable_recv(socket, server_md5, EVP_MD_size(EVP_md5())) < 0) {
        printf("Erro ao receber hash MD5 do servidor\n");
    } else {
        EVP_DigestFinal_ex(md5_context, local_md5, &md5_len);
        if(memcmp(server_md5, local_md5, md5_len) == 0) {
            printf("Verificação MD5: OK\n");
        } else {
            printf("Verificação MD5: FALHA - Arquivo pode estar corrompido\n");
        }
    }

    EVP_MD_CTX_free(md5_context);
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ip_servidor> <porta_servidor>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int client_socket;
    struct sockaddr_in server_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao conectar ao servidor");
        exit(EXIT_FAILURE);
    }

    printf("Conectado ao servidor %s:%d\n", server_ip, port);
    printf("Comandos disponíveis:\n");
    printf("  get <nome_arquivo> - Baixa um arquivo do servidor\n");
    printf("  exit - Encerra a conexão\n");

    char command[256];
    while (1) {
        printf("> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';

        if (strlen(command) == 0) continue;

        if (strcmp(command, "exit") == 0) {
            send(client_socket, command, strlen(command), 0);
            printf("Encerrando cliente...\n");
            break;
        }

        if (strncmp(command, "get ", 4) == 0) {
            send(client_socket, command, strlen(command), 0);
            char *filename = command + 4;
            printf("Baixando arquivo: %s\n", filename);
            receive_file(client_socket, filename);
        } else {
            printf("Comando desconhecido\n");
        }
    }

    close(client_socket);
    return 0;
}