#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/stat.h>
#include <openssl/md5.h>
#include <errno.h>
#include <sys/time.h>

#define BUFFER_SIZE 4096
#define LOG_FILE "client_tcp.log"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

void log_transfer(const char *filename, double duration, double speed, 
                 const char *md5_received, const char *md5_calculated, 
                 const char *server_ip, long file_size) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        perror("Erro ao abrir arquivo de log");
        return;
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(log_file, "[%s] Transferência concluída:\n", timestamp);
    fprintf(log_file, "  Servidor: %s\n", server_ip);
    fprintf(log_file, "  Arquivo: %s\n", filename);
    fprintf(log_file, "  Tamanho: %ld bytes\n", file_size);
    fprintf(log_file, "  Tempo: %.6f segundos\n", duration);
    fprintf(log_file, "  Velocidade: %.2f KB/s\n", speed);
    fprintf(log_file, "  MD5 recebido: %s\n", md5_received);
    fprintf(log_file, "  MD5 calculado: %s\n", md5_calculated);
    fprintf(log_file, "  Status: %s\n", strcmp(md5_received, md5_calculated) == 0 ? "OK" : "FALHA");
    fprintf(log_file, "----------------------------------------\n");
    fclose(log_file);
}

void calculate_md5(const char *filename, char *md5sum) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        strcpy(md5sum, "Arquivo não encontrado");
        return;
    }

    MD5_CTX md5Context;
    MD5_Init(&md5Context);

    unsigned char buffer[BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, file)) != 0) {
        MD5_Update(&md5Context, buffer, bytesRead);
    }

    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_Final(digest, &md5Context);

    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(&md5sum[i*2], "%02x", (unsigned int)digest[i]);
    }
    md5sum[MD5_DIGEST_LENGTH*2] = '\0';

    fclose(file);
}

void receive_file(int sockfd, const char *filename, const char *server_ip) {
    char meta_buffer[sizeof(long) + MD5_DIGEST_LENGTH*2 + 1];
    int n = recv(sockfd, meta_buffer, sizeof(meta_buffer), 0);
    if (n <= 0) {
        perror("Erro ao receber metadados");
        return;
    }

    if (strncmp(meta_buffer, "ERRO:", 5) == 0) {
        printf("%s\n", meta_buffer);
        return;
    }

    long file_size;
    char md5_received[MD5_DIGEST_LENGTH*2 + 1];
    memcpy(&file_size, meta_buffer, sizeof(long));
    memcpy(md5_received, meta_buffer + sizeof(long), MD5_DIGEST_LENGTH*2 + 1);

    printf("Recebendo arquivo '%s' (%ld bytes)...\n", filename, file_size);

    char *last_slash = strrchr(filename, '/');
    if (last_slash != NULL) {
        char path[256];
        strncpy(path, filename, last_slash - filename);
        path[last_slash - filename] = '\0';
        
        char command[512];
        snprintf(command, sizeof(command), "mkdir -p %s", path);
        system(command);
    }

    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Erro ao criar arquivo local");
        return;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    char buffer[BUFFER_SIZE];
    long total_bytes_received = 0;
    int timeout_counter = 0;
    const int max_timeout_retries = 3;

    while (total_bytes_received < file_size) {
        n = recv(sockfd, buffer, BUFFER_SIZE, 0);
        
        if (n == 0) {
            printf("Conexão fechada pelo servidor\n");
            break;
        } else if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                timeout_counter++;
                if (timeout_counter >= max_timeout_retries) {
                    printf("Timeout: Nenhum dado recebido após %d tentativas\n", max_timeout_retries);
                    break;
                }
                printf("Timeout: Tentando novamente (%d/%d)\n", timeout_counter, max_timeout_retries);
                continue;
            } else {
                perror("Erro ao receber dados do arquivo");
                break;
            }
        }

        timeout_counter = 0;
        
        size_t bytes_written = fwrite(buffer, 1, n, file);
        if (bytes_written != n) {
            perror("Erro ao escrever no arquivo");
            break;
        }
        total_bytes_received += n;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    fclose(file);

    if (total_bytes_received != file_size) {
        printf("Aviso: Transferência incompleta. Recebidos %ld/%ld bytes\n", 
               total_bytes_received, file_size);
    }

    double duration = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double speed_kbs = (total_bytes_received / 1024.0) / duration;

    char md5_calculated[MD5_DIGEST_LENGTH*2 + 1];
    if (total_bytes_received == file_size) {
        calculate_md5(filename, md5_calculated);
    } else {
        strcpy(md5_calculated, "Transferência incompleta");
    }

    log_transfer(filename, duration, speed_kbs, md5_received, md5_calculated, 
                server_ip, total_bytes_received);

    printf("Transferência concluída:\n");
    printf("  Tamanho: %ld bytes\n", total_bytes_received);
    printf("  Tempo: %.6f segundos\n", duration);
    printf("  Velocidade: %.2f KB/s\n", speed_kbs);
    printf("  MD5 recebido: %s\n", md5_received);
    printf("  MD5 calculado: %s\n", md5_calculated);
    
    if (total_bytes_received == file_size) {
        printf("  Status: %s\n", strcmp(md5_received, md5_calculated) == 0 ? "OK" : "FALHA");
    } else {
        printf("  Status: Transferência incompleta\n");
    }
}

void receive_file_list(int sockfd) {
    char buffer[BUFFER_SIZE];
    int n = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    if (n > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
    } else if (n == 0) {
        printf("Conexão fechada pelo servidor\n");
    } else {
        perror("Erro ao receber lista de arquivos");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <IP do servidor> <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Erro ao configurar timeout de recebimento");
    }
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Erro ao configurar timeout de envio");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port);

    printf("Conectando ao servidor %s:%d...\n", server_ip, port);
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao conectar ao servidor");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Conectado ao servidor TCP %s:%d\n", server_ip, port);
    printf("Comandos disponíveis:\n");
    printf("  get <nome_arquivo> - Solicitar um arquivo\n");
    printf("  list - Listar arquivos disponíveis\n");
    printf("  exit - Encerrar o cliente\n");

    char command[BUFFER_SIZE];
    while (1) {
        printf("\n> ");
        if (fgets(command, BUFFER_SIZE, stdin) == NULL) {
            printf("\nEntrada inválida. Encerrando...\n");
            break;
        }

        command[strcspn(command, "\n")] = '\0';

        if (strncmp(command, "get ", 4) == 0 && strlen(command) > 4) {
            if (send(sockfd, command, strlen(command) + 1, 0) < 0) {
                perror("Erro ao enviar comando");
                continue;
            }
            
            char filename[256];
            strncpy(filename, command + 4, sizeof(filename) - 1);
            filename[sizeof(filename) - 1] = '\0';

            receive_file(sockfd, filename, server_ip);
        } else if (strcmp(command, "exit") == 0) {
            printf("Encerrando cliente...\n");
            break;
        } else if (strcmp(command, "list") == 0) {
            if (send(sockfd, command, strlen(command) + 1, 0) < 0) {
                perror("Erro ao enviar comando");
                continue;
            }
            receive_file_list(sockfd);
        } else {
            printf("Comando inválido. Use 'get <arquivo>', 'list' ou 'exit'\n");
        }
    }

    close(sockfd);
    return 0;
}