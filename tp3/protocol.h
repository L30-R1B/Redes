#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h> 


#define PAYLOAD_SIZE 1024
#define TIMEOUT_SEC 1       
#define MAX_RETRIES 5       


#define TYPE_START 1
#define TYPE_DATA  2
#define TYPE_ACK   3
#define TYPE_END   4
#define TYPE_END_ACK 5
#define TYPE_START_ACK 6





typedef struct __attribute__((__packed__)) {
    uint32_t seq_num;      
    uint32_t ack_num;      
    uint8_t  type;         
    uint8_t  checksum;     
    uint16_t payload_size; 
    char     payload[PAYLOAD_SIZE]; 
} packet;

#define MAX_PACKET_SIZE (sizeof(packet))

/**
 * @brief Calcula o checksum de 8 bits para um pacote.
 * A função soma todos os bytes do pacote (com o campo checksum zerado)
 * e retorna o resultado truncado para 8 bits.
 * @param pkt Ponteiro para o pacote a ser verificado.
 * @return O checksum de 8 bits calculado.
 */
static inline uint8_t calculate_checksum(const packet* pkt) {
    
    packet temp_pkt = *pkt;
    temp_pkt.checksum = 0; 

    uint8_t* p = (uint8_t*)&temp_pkt;
    unsigned int sum = 0;
    
    
    size_t size_to_check = offsetof(packet, payload) + pkt->payload_size;

    for (size_t i = 0; i < size_to_check; ++i) {
        sum += p[i];
    }

    
    return sum & 0xFF;
}

/**
 * @brief Cria e inicializa um pacote.
 * @param type Tipo do pacote.
 * @param seq_num Número de sequência.
 * @param ack_num Número de ACK.
 * @param payload_size Tamanho da carga útil.
 * @param payload Ponteiro para a carga útil.
 * @return Um pacote inicializado.
 */
static inline packet create_packet(uint8_t type, uint32_t seq_num, uint32_t ack_num, uint16_t payload_size, const char* payload) {
    packet pkt;
    memset(&pkt, 0, sizeof(packet));
    pkt.type = type;
    pkt.seq_num = seq_num;
    pkt.ack_num = ack_num;
    pkt.payload_size = payload_size;
    if (payload && payload_size > 0) {
        memcpy(pkt.payload, payload, payload_size);
    }
    
    pkt.checksum = calculate_checksum(&pkt);
    return pkt;
}

#endif 