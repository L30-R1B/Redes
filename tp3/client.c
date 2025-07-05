
#define _POSIX_C_SOURCE 200809L

#include "protocol.h"

int verbose = 0; 

void print_usage(const char* prog_name) {
    fprintf(stderr, "Uso: %s -h <host> -p <porta> -f <arquivo> [-v]\n", prog_name);
}


int main(int argc, char *argv[]) {
    int opt;
    char *host = NULL;
    int port = -1;
    char *filename = NULL;

    
    while ((opt = getopt(argc, argv, "h:p:f:v"))!= -1) {
        switch (opt) {
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'f':
                filename = optarg;
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    
    if (!host || port == -1 || !filename) {
        fprintf(stderr, "Erro: Host, porta e nome do arquivo são obrigatórios.\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Erro ao abrir o arquivo de entrada");
        exit(EXIT_FAILURE);
    }

    int sockfd;
    struct sockaddr_in servaddr;

    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Falha na criação do socket");
        exit(EXIT_FAILURE);
    }

    
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Erro ao configurar o timeout do socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(host);

    
    long total_packets_sent = 0;
    long retransmissions = 0;
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    
    packet start_pkt = create_packet(TYPE_START, 0, 0, 0, NULL);
    packet recv_pkt;
    int retries = 0;
    while (retries < MAX_RETRIES) {
        if (verbose) printf("Enviando START...\n");
        sendto(sockfd, &start_pkt, sizeof(start_pkt), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        total_packets_sent++;

        ssize_t n = recvfrom(sockfd, &recv_pkt, MAX_PACKET_SIZE, 0, NULL, NULL);
        if (n > 0 && recv_pkt.type == TYPE_START_ACK) {
            if (verbose) printf("Recebido START_ACK. Iniciando transferência de dados.\n");
            break;
        } else if (n < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) { 
            if (verbose) printf("Timeout esperando por START_ACK. Tentando novamente...\n");
            retries++;
            retransmissions++;
        }
    }
    if (retries == MAX_RETRIES) {
        fprintf(stderr, "Erro: Servidor não respondeu ao START. Abortando.\n");
        fclose(file);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    
    char buffer[PAYLOAD_SIZE]; 
    uint32_t seq_num = 0;
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, PAYLOAD_SIZE, file)) > 0) {
        packet data_pkt = create_packet(TYPE_DATA, seq_num, 0, bytes_read, buffer);
        retries = 0;
        
        while (retries < MAX_RETRIES) {
            if (verbose) printf("Enviando DATA com seq=%u (%zu bytes)\n", seq_num, bytes_read);
            sendto(sockfd, &data_pkt, sizeof(data_pkt), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
            total_packets_sent++;

            ssize_t n = recvfrom(sockfd, &recv_pkt, MAX_PACKET_SIZE, 0, NULL, NULL);
            if (n > 0 && recv_pkt.type == TYPE_ACK && recv_pkt.ack_num == seq_num) {
                if (verbose) printf("Recebido ACK para seq=%u.\n", seq_num);
                break; 
            } else if (n < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) { 
                if (verbose) printf("Timeout esperando por ACK de seq=%u. Retransmitindo...\n", seq_num);
                retries++;
                retransmissions++;
            } else {
                if (verbose && n > 0) printf("Recebido pacote inesperado (type=%d, ack_num=%u) enquanto esperava ACK para seq=%u.\n", recv_pkt.type, recv_pkt.ack_num, seq_num);
            }
        }

        if (retries == MAX_RETRIES) {
            fprintf(stderr, "Erro: Falha ao enviar pacote com seq=%u após %d tentativas. Abortando.\n", seq_num, MAX_RETRIES);
            fclose(file);
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        seq_num = 1 - seq_num; 
    }

    
    packet end_pkt = create_packet(TYPE_END, 0, 0, 0, NULL);
    retries = 0;
    while (retries < MAX_RETRIES) {
        if (verbose) printf("Enviando END...\n");
        sendto(sockfd, &end_pkt, sizeof(end_pkt), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        total_packets_sent++;
        
        ssize_t n = recvfrom(sockfd, &recv_pkt, MAX_PACKET_SIZE, 0, NULL, NULL);
        if (n > 0 && recv_pkt.type == TYPE_END_ACK) {
            if (verbose) printf("Recebido END_ACK. Transferência concluída com sucesso.\n");
            break;
        } else if (n < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) { 
            if (verbose) printf("Timeout esperando por END_ACK. Tentando novamente...\n");
            retries++;
            retransmissions++;
        }
    }
    if (retries == MAX_RETRIES) {
        fprintf(stderr, "Aviso: Não foi possível confirmar o fim da transmissão com o servidor.\n");
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);

    
    double time_elapsed = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    double throughput = (time_elapsed > 0) ? (file_size / 1024.0) / time_elapsed : 0;
    
    printf("\n--- Estatísticas do Cliente ---\n");
    printf("Arquivo: %s (%ld bytes)\n", filename, file_size);
    printf("Tempo total de transferência: %.3f segundos\n", time_elapsed);
    printf("Total de pacotes enviados (dados + controle): %ld\n", total_packets_sent);
    printf("Total de retransmissões: %ld\n", retransmissions);
    printf("Taxa de transferência efetiva: %.2f KB/s\n", throughput);
    printf("--------------------------------\n");

    fclose(file);
    close(sockfd);
    return 0;
}