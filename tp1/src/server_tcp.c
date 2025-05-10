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
#define LOG_FILE "server_tcp.log"
#define MAX_PATH_LEN 1024
#define MAX_CLIENTS 5

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

char root_dir[MAX_PATH_LEN] = ".";

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

void send_file(int sockfd, const char *filename, const char *client_ip) {
    char full_path[MAX_PATH_LEN * 2];
    if (snprintf(full_path, sizeof(full_path), "%s/%s", root_dir, filename) >= sizeof(full_path)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "ERRO: Caminho do arquivo muito longo");
        send(sockfd, error_msg, strlen(error_msg) + 1, 0);
        return;
    }

    FILE *file = fopen(full_path, "rb");
    if (!file) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "ERRO: Arquivo '%s' não encontrado no servidor", filename);
        send(sockfd, error_msg, strlen(error_msg) + 1, 0);
        return;
    }

    struct stat file_stat;
    if (fstat(fileno(file), &file_stat) < 0) {
        perror("Erro ao obter estatísticas do arquivo");
        fclose(file);
        return;
    }
    long file_size = file_stat.st_size;

    char md5sum[MD5_DIGEST_LENGTH*2 + 1];
    calculate_md5(full_path, md5sum);

    char meta_buffer[sizeof(long) + MD5_DIGEST_LENGTH*2 + 1];
    memcpy(meta_buffer, &file_size, sizeof(long));
    memcpy(meta_buffer + sizeof(long), md5sum, MD5_DIGEST_LENGTH*2 + 1);
    
    send(sockfd, meta_buffer, sizeof(meta_buffer), 0);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (send(sockfd, buffer, bytes_read, 0) < 0) {
            perror("Erro ao enviar dados do arquivo");
            break;
        }
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
}

void send_file_list(int sockfd) {
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
        send(sockfd, file_list, strlen(file_list) + 1, 0);
    } else {
        char *error_msg = "ERRO: Não foi possível listar arquivos";
        send(sockfd, error_msg, strlen(error_msg) + 1, 0);
    }
}

void handle_client(int sockfd, const char *client_ip) {
    char command[BUFFER_SIZE];
    int n;

    while ((n = recv(sockfd, command, BUFFER_SIZE - 1, 0)) > 0) {
        command[n] = '\0';
        printf("Comando recebido de %s: %s\n", client_ip, command);

        if (strncmp(command, "get ", 4) == 0) {
            char filename[256];
            strncpy(filename, command + 4, sizeof(filename) - 1);
            filename[sizeof(filename) - 1] = '\0';

            char *newline = strchr(filename, '\n');
            if (newline) *newline = '\0';

            send_file(sockfd, filename, client_ip);
        } else if (strncmp(command, "list", 4) == 0) {
            send_file_list(sockfd);
        } else {
            char *response = "Comando inválido. Use 'get <arquivo>' ou 'list'";
            send(sockfd, response, strlen(response) + 1, 0);
        }
    }

    close(sockfd);
    printf("Conexão com %s encerrada\n", client_ip);
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

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Servidor TCP iniciado na porta %d. Aguardando conexões...\n", PORT);

    while (1) {
        char client_ip[INET_ADDRSTRLEN];
        
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("accept");
            continue;
        }

        inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Nova conexão de %s\n", client_ip);

        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd);
            handle_client(new_socket, client_ip);
            exit(0);
        } else if (pid < 0) {
            perror("fork");
            close(new_socket);
        } else {
            close(new_socket);
        }
    }

    close(server_fd);
    return 0;
}