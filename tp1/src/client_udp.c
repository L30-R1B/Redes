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
#define LOG_FILE "client_udp.log"
#define HEADER_SIZE 5
#define TIMEOUT_MS 1000
#define MAX_RETRIES 3

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

typedef struct {
    int packet_number;
    char is_last;
    char data[BUFFER_SIZE];
} Packet;

void log_transfer(const char *filename, double duration, double speed, const char *md5_received, const char *md5_calculated, const char *server_ip, long file_size, int packets_received, int packets_expected) {
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
    fprintf(log_file, "  Pacotes esperados: %d\n", packets_expected);
    fprintf(log_file, "  Pacotes recebidos: %d\n", packets_received);
    fprintf(log_file, "  Pacotes perdidos: %d (%.2f%%)\n", 
            packets_expected - packets_received, 
            (packets_expected - packets_received) * 100.0 / packets_expected);
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

void receive_file(int sockfd, struct sockaddr_in server_addr, const char *filename, const char *server_ip) {
    // Configurar timeout
    struct timeval tv;
    tv.tv_sec = TIMEOUT_MS / 1000;
    tv.tv_usec = (TIMEOUT_MS % 1000) * 1000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Receber metadados (tamanho e MD5)
    char meta_buffer[sizeof(long) + MD5_DIGEST_LENGTH*2 + 1];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    int n = recvfrom(sockfd, meta_buffer, sizeof(meta_buffer), 0,
                    (struct sockaddr *)&from_addr, &from_len);
    if (n <= 0) {
        perror("Erro ao receber metadados");
        return;
    }

    // Verificar se é mensagem de erro
    if (strncmp(meta_buffer, "ERRO:", 5) == 0) {
        printf("%s\n", meta_buffer);
        return;
    }

    long file_size;
    char md5_received[MD5_DIGEST_LENGTH*2 + 1];
    memcpy(&file_size, meta_buffer, sizeof(long));
    memcpy(md5_received, meta_buffer + sizeof(long), MD5_DIGEST_LENGTH*2 + 1);

    printf("Recebendo arquivo '%s' (%ld bytes)...\n", filename, file_size);

    // Criar diretórios se necessário
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

    Packet *received_packets = malloc((file_size / BUFFER_SIZE + 2) * sizeof(Packet));
    int total_packets_expected = (file_size + BUFFER_SIZE - 1) / BUFFER_SIZE;
    int total_packets_received = 0;
    int packets_received = 0;

    while (packets_received < total_packets_expected) {
        Packet packet;
        n = recvfrom(sockfd, &packet, sizeof(Packet), 0,
                    (struct sockaddr *)&from_addr, &from_len);
        
        if (n < 0) {
            printf("Timeout aguardando pacotes...\n");
            break;
        }

        if (n < sizeof(int) + 1) {
            printf("Pacote inválido recebido (tamanho muito pequeno)\n");
            continue;
        }

        // Verificar se já recebemos este pacote
        int already_received = 0;
        for (int i = 0; i < total_packets_received; i++) {
            if (received_packets[i].packet_number == packet.packet_number) {
                already_received = 1;
                break;
            }
        }

        if (!already_received) {
            // Armazenar pacote
            received_packets[total_packets_received] = packet;
            total_packets_received++;
            packets_received++;
            
            if (packet.is_last) {
                break;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    
    // Ordenar pacotes por número
    for (int i = 0; i < total_packets_received - 1; i++) {
        for (int j = i + 1; j < total_packets_received; j++) {
            if (received_packets[i].packet_number > received_packets[j].packet_number) {
                Packet temp = received_packets[i];
                received_packets[i] = received_packets[j];
                received_packets[j] = temp;
            }
        }
    }

    // Escrever arquivo
    for (int i = 0; i < total_packets_received; i++) {
        size_t bytes_to_write = (i == total_packets_received - 1) ? 
                              (file_size % BUFFER_SIZE) : BUFFER_SIZE;
        if (bytes_to_write == 0) bytes_to_write = BUFFER_SIZE;
        
        fwrite(received_packets[i].data, 1, bytes_to_write, file);
    }

    fclose(file);
    free(received_packets);

    double duration = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double speed_kbs = (file_size / 1024.0) / duration;

    char md5_calculated[MD5_DIGEST_LENGTH*2 + 1];
    calculate_md5(filename, md5_calculated);

    log_transfer(filename, duration, speed_kbs, md5_received, md5_calculated, 
                server_ip, file_size, total_packets_received, total_packets_expected);

    printf("Transferência concluída:\n");
    printf("  Tamanho: %ld bytes\n", file_size);
    printf("  Tempo: %.6f segundos\n", duration);
    printf("  Velocidade: %.2f KB/s\n", speed_kbs);
    printf("  Pacotes esperados: %d\n", total_packets_expected);
    printf("  Pacotes recebidos: %d\n", total_packets_received);
    printf("  Pacotes perdidos: %d (%.2f%%)\n", 
           total_packets_expected - total_packets_received,
           (total_packets_expected - total_packets_received) * 100.0 / total_packets_expected);
    printf("  MD5 recebido: %s\n", md5_received);
    printf("  MD5 calculado: %s\n", md5_calculated);
    
    if (total_packets_received == total_packets_expected) {
        printf("  Status: %s\n", strcmp(md5_received, md5_calculated) == 0 ? "OK" : "FALHA");
    } else {
        printf("  Status: Transferência incompleta\n");
    }
}

void receive_file_list(int sockfd, struct sockaddr_in server_addr) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    int n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                    (struct sockaddr *)&from_addr, &from_len);
    if (n > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
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

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port);

    printf("Conectado ao servidor UDP %s:%d\n", server_ip, port);
    printf("Comandos disponíveis:\n");
    printf("  get <nome_arquivo> - Solicitar um arquivo\n");
    printf("  list - Listar arquivos disponíveis\n");
    printf("  exit - Encerrar o cliente\n");

    char command[BUFFER_SIZE];
    while (1) {
        printf("\n> ");
        if (fgets(command, BUFFER_SIZE, stdin) == NULL) {
            break;
        }

        command[strcspn(command, "\n")] = '\0';

        if (strncmp(command, "get ", 4) == 0 && strlen(command) > 4) {
            sendto(sockfd, command, strlen(command) + 1, 0,
                  (struct sockaddr *)&server_addr, sizeof(server_addr));
            
            char filename[256];
            strncpy(filename, command + 4, sizeof(filename) - 1);
            filename[sizeof(filename) - 1] = '\0';

            receive_file(sockfd, server_addr, filename, server_ip);
        } else if (strcmp(command, "exit") == 0) {
            printf("Encerrando cliente...\n");
            break;
        } else if (strcmp(command, "list") == 0) {
            sendto(sockfd, command, strlen(command) + 1, 0,
                  (struct sockaddr *)&server_addr, sizeof(server_addr));
            receive_file_list(sockfd, server_addr);
        } else {
            printf("Comando inválido. Use 'get <arquivo>', 'list' ou 'exit'\n");
        }
    }

    close(sockfd);
    return 0;
}