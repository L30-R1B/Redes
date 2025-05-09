#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <openssl/md5.h>
#include <libgen.h>
#include <errno.h>
#include <sys/time.h>

#define BUFFER_SIZE 4096
#define PORT 8080
#define LOG_FILE "server_udp.log"
#define MAX_PATH_LEN 1024
#define HEADER_SIZE 5  // 4 bytes para número do pacote + 1 byte para flag de fim
#define TIMEOUT_MS 1000

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

char root_dir[MAX_PATH_LEN] = ".";
int packet_counter = 0;

typedef struct {
    int packet_number;
    char is_last;
    char data[BUFFER_SIZE];
} Packet;

void log_transfer(const char *filename, double duration, double speed, const char *md5, const char *client_ip, long file_size) {
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
    fprintf(log_file, "  Cliente: %s\n", client_ip);
    fprintf(log_file, "  Arquivo: %s\n", filename);
    fprintf(log_file, "  Tamanho: %ld bytes\n", file_size);
    fprintf(log_file, "  Tempo: %.6f segundos\n", duration);
    fprintf(log_file, "  Velocidade: %.2f KB/s\n", speed);
    fprintf(log_file, "  MD5: %s\n", md5);
    fprintf(log_file, "  Pacotes enviados: %d\n", packet_counter);
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

int is_path_safe(const char *requested_path) {
    char resolved_path[MAX_PATH_LEN];
    if (realpath(requested_path, resolved_path) == NULL) {
        return 0;
    }
    
    return strncmp(root_dir, resolved_path, strlen(root_dir)) == 0;
}

void send_file(int sockfd, struct sockaddr_in client_addr, const char *filename, const char *client_ip) {
    char full_path[MAX_PATH_LEN * 2];
    if (snprintf(full_path, sizeof(full_path), "%s/%s", root_dir, filename) >= sizeof(full_path)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "ERRO: Caminho do arquivo muito longo");
        sendto(sockfd, error_msg, strlen(error_msg) + 1, 0, 
              (struct sockaddr *)&client_addr, sizeof(client_addr));
        return;
    }

    FILE *file = fopen(full_path, "rb");
    if (!file) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "ERRO: Arquivo '%s' não encontrado no servidor", filename);
        sendto(sockfd, error_msg, strlen(error_msg) + 1, 0,
              (struct sockaddr *)&client_addr, sizeof(client_addr));
        return;
    }

    struct stat file_stat;
    if (fstat(fileno(file), &file_stat) < 0) {
        perror("Erro ao obter estatísticas do arquivo");
        fclose(file);
        return;
    }
    long file_size = file_stat.st_size;

    // Calcular MD5 antes de enviar
    char md5sum[MD5_DIGEST_LENGTH*2 + 1];
    calculate_md5(full_path, md5sum);

    // Enviar metadados primeiro (tamanho e MD5)
    char meta_buffer[sizeof(long) + MD5_DIGEST_LENGTH*2 + 1];
    memcpy(meta_buffer, &file_size, sizeof(long));
    memcpy(meta_buffer + sizeof(long), md5sum, MD5_DIGEST_LENGTH*2 + 1);
    
    sendto(sockfd, meta_buffer, sizeof(meta_buffer), 0,
          (struct sockaddr *)&client_addr, sizeof(client_addr));

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    packet_counter = 0;

    Packet packet;
    size_t bytes_read;
    while ((bytes_read = fread(packet.data, 1, BUFFER_SIZE, file)) > 0) {
        packet.packet_number = packet_counter;
        packet.is_last = (bytes_read < BUFFER_SIZE) ? 1 : 0;
        
        sendto(sockfd, &packet, sizeof(int) + 1 + bytes_read, 0,
              (struct sockaddr *)&client_addr, sizeof(client_addr));
        
        packet_counter++;
        usleep(1000); // Pequena pausa para evitar congestionamento
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    fclose(file);

    double duration = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double speed_kbs = (file_size / 1024.0) / duration;

    log_transfer(filename, duration, speed_kbs, md5sum, client_ip, file_size);

    printf("Arquivo '%s' enviado com sucesso para %s\n", filename, client_ip);
    printf("  Tamanho: %ld bytes\n", file_size);
    printf("  Tempo: %.6f segundos\n", duration);
    printf("  Velocidade: %.2f KB/s\n", speed_kbs);
    printf("  MD5: %s\n", md5sum);
    printf("  Pacotes enviados: %d\n", packet_counter);
}

void send_file_list(int sockfd, struct sockaddr_in client_addr) {
    DIR *dir;
    struct dirent *ent;
    char file_list[BUFFER_SIZE] = "Arquivos disponíveis:\n";
    
    if ((dir = opendir(root_dir)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) {
                strncat(file_list, "  ", BUFFER_SIZE - strlen(file_list) - 1);
                strncat(file_list, ent->d_name, BUFFER_SIZE - strlen(file_list) - 1);
                strncat(file_list, "\n", BUFFER_SIZE - strlen(file_list) - 1);
            }
        }
        closedir(dir);
        sendto(sockfd, file_list, strlen(file_list) + 1, 0,
              (struct sockaddr *)&client_addr, sizeof(client_addr));
    } else {
        char *error_msg = "ERRO: Não foi possível listar arquivos";
        sendto(sockfd, error_msg, strlen(error_msg) + 1, 0,
              (struct sockaddr *)&client_addr, sizeof(client_addr));
    }
}

void handle_client_command(int sockfd, struct sockaddr_in client_addr, char *command) {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

    printf("Comando recebido de %s: %s\n", client_ip, command);

    if (strncmp(command, "get ", 4) == 0) {
        char filename[256];
        strncpy(filename, command + 4, sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = '\0';

        char *newline = strchr(filename, '\n');
        if (newline) *newline = '\0';

        send_file(sockfd, client_addr, filename, client_ip);
    } else if (strncmp(command, "list", 4) == 0) {
        send_file_list(sockfd, client_addr);
    } else {
        char *response = "Comando inválido. Use 'get <arquivo>' ou 'list'";
        sendto(sockfd, response, strlen(response) + 1, 0,
              (struct sockaddr *)&client_addr, sizeof(client_addr));
    }
}

int main(int argc, char *argv[]) {
    if (argc > 2) {
        printf("Uso: %s [diretorio_raiz]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (argc == 2) {
        if (realpath(argv[1], root_dir) == NULL) {
            perror("Erro ao obter caminho absoluto do diretório raiz");
            exit(EXIT_FAILURE);
        }
        
        DIR *dir = opendir(root_dir);
        if (dir == NULL) {
            perror("Erro ao acessar diretório raiz");
            exit(EXIT_FAILURE);
        }
        closedir(dir);
        
        printf("Diretório raiz configurado para: %s\n", root_dir);
    }

    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao vincular socket");
        exit(EXIT_FAILURE);
    }

    printf("Servidor UDP iniciado na porta %d. Aguardando comandos...\n", PORT);

    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                        (struct sockaddr *)&client_addr, &addr_len);
        if (n > 0) {
            buffer[n] = '\0';
            handle_client_command(sockfd, client_addr, buffer);
        }
    }

    close(sockfd);
    return 0;
}