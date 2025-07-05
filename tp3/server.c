
#define _POSIX_C_SOURCE 200809L

#include "protocol.h"


long total_packets_received = 0;
long duplicate_packets = 0;
long checksum_errors = 0;
long total_bytes_written = 0;

int verbose = 0; 

/**
 * @brief Simula a perda de um pacote com base em uma taxa de perda.
 * @param loss_rate A taxa de perda em porcentagem (0-100).
 * @return 1 se o pacote deve ser descartado, 0 caso contrário.
 */
int should_drop_packet(int loss_rate) {
    if (loss_rate > 0 && (rand() % 100) < loss_rate) {
        return 1;
    }
    return 0;
}

void print_usage(const char* prog_name) {
    fprintf(stderr, "Uso: %s -p <porta> -l <taxa_perda> [-v]\n", prog_name);
}


int main(int argc, char *argv[]) {
    int opt;
    int port = -1;
    int loss_rate = 0;

    
    while ((opt = getopt(argc, argv, "p:l:v"))!= -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'l':
                loss_rate = atoi(optarg);
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (port == -1) {
        fprintf(stderr, "Erro: A porta do servidor é obrigatória.\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    
    
    if (loss_rate < 0 || loss_rate > 100) {
        fprintf(stderr, "Erro: A taxa de perda deve ser entre 0 e 100.\n");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL)); 

    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Falha na criação do socket");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Falha no bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor ouvindo na porta %d com taxa de perda de %d%%\n", port, loss_rate);

    packet recv_pkt;
    socklen_t len = sizeof(cliaddr);
    uint32_t expected_seq_num = 0;
    FILE *output_file = NULL;
    
    
    while (1) {
        ssize_t n = recvfrom(sockfd, &recv_pkt, MAX_PACKET_SIZE, 0, (struct sockaddr *)&cliaddr, &len);
        if (n < 0) {
            perror("Erro no recvfrom");
            continue;
        }

        total_packets_received++;

        
        if (should_drop_packet(loss_rate)) {
            if (verbose) printf("PACOTE SIMULADO PERDIDO (seq=%u, type=%d)\n", recv_pkt.seq_num, recv_pkt.type);
            continue;
        }

        uint8_t received_checksum = recv_pkt.checksum;
        uint8_t calculated_checksum = calculate_checksum(&recv_pkt);

        
        if (received_checksum!= calculated_checksum) {
            checksum_errors++;
            if (verbose) printf("ERRO DE CHECKSUM: recebido=%u, calculado=%u. Pacote descartado.\n", received_checksum, calculated_checksum);
            continue;
        }

        
        switch (recv_pkt.type) {
            case TYPE_START:
                if (verbose) printf("Recebido START. Enviando START_ACK.\n");
                output_file = fopen("received_file.txt", "wb");
                if (!output_file) {
                    perror("Não foi possível criar o arquivo de saída");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }
                expected_seq_num = 0; 
                packet start_ack = create_packet(TYPE_START_ACK, 0, 0, 0, NULL);
                sendto(sockfd, &start_ack, sizeof(start_ack), 0, (const struct sockaddr *)&cliaddr, len);
                break;

            case TYPE_DATA:
                if (recv_pkt.seq_num == expected_seq_num) {
                    if (verbose) printf("Recebido DATA com seq=%u. Enviando ACK %u.\n", recv_pkt.seq_num, recv_pkt.seq_num);
                    fwrite(recv_pkt.payload, 1, recv_pkt.payload_size, output_file);
                    total_bytes_written += recv_pkt.payload_size;
                    
                    packet ack_pkt = create_packet(TYPE_ACK, 0, recv_pkt.seq_num, 0, NULL);
                    sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0, (const struct sockaddr *)&cliaddr, len);
                    expected_seq_num = 1 - expected_seq_num; 
                } else {
                    duplicate_packets++;
                    if (verbose) printf("Recebido DATA duplicado com seq=%u (esperado=%u). Reenviando ACK %u.\n", recv_pkt.seq_num, expected_seq_num, recv_pkt.seq_num);
                    packet ack_pkt = create_packet(TYPE_ACK, 0, recv_pkt.seq_num, 0, NULL);
                    sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0, (const struct sockaddr *)&cliaddr, len);
                }
                break;

            case TYPE_END:
                if (verbose) printf("Recebido END. Enviando END_ACK e finalizando.\n");
                if (output_file) {
                    fclose(output_file);
                    output_file = NULL;
                }

                packet end_ack_pkt = create_packet(TYPE_END_ACK, 0, 0, 0, NULL);
                sendto(sockfd, &end_ack_pkt, sizeof(end_ack_pkt), 0, (const struct sockaddr *)&cliaddr, len);

                
                printf("\n--- Estatísticas do Servidor ---\n");
                printf("Total de pacotes recebidos: %ld\n", total_packets_received);
                printf("Pacotes duplicados: %ld\n", duplicate_packets);
                printf("Erros de checksum: %ld\n", checksum_errors);
                printf("Total de bytes escritos: %ld\n", total_bytes_written);
                printf("--------------------------------\n");
                
                
                total_packets_received = 0;
                duplicate_packets = 0;
                checksum_errors = 0;
                total_bytes_written = 0;
                break;
        }
    }

    close(sockfd);
    return 0;
}
