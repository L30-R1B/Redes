#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>

// Tamanho padrão do buffer para escrita
#define BUFFER_SIZE 4096

// Caracteres possíveis para geração aleatória
const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ,.;:!?(){}[]<>/*-+=";

void generate_random_text(char *buffer, size_t size) {
    for (size_t i = 0; i < size - 1; i++) {
        // Adiciona quebras de linha e parágrafos ocasionalmente
        if (i > 0 && i % 80 == 0) {
            if (rand() % 10 < 3) {  // 30% de chance de quebra de parágrafo
                buffer[i++] = '\n';
                buffer[i++] = '\n';
            } else {                // 70% de chance de quebra de linha simples
                buffer[i++] = '\n';
            }
        }
        
        // Seleciona caractere aleatório do charset
        buffer[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    buffer[size - 1] = '\0';
}

void generate_random_words(char *buffer, size_t size) {
    const char *words[] = {
        "redes", "computadores", "protocolo", "TCP", "UDP", "arquivo", "transferencia",
        "cliente", "servidor", "socket", "porta", "IP", "dados", "pacote", "bytes",
        "velocidade", "tempo", "conexao", "internet", "fibra", "optica", "roteador",
        "switch", "broadcast", "download", "upload", "largura", "banda", "ping", "latencia"
    };
    const int word_count = sizeof(words) / sizeof(words[0]);
    
    size_t pos = 0;
    while (pos < size - 1) {
        // Seleciona palavra aleatória
        const char *word = words[rand() % word_count];
        size_t word_len = strlen(word);
        
        // Verifica se há espaço para a palavra + espaço/queabra de linha
        if (pos + word_len + 2 >= size) {
            break;
        }
        
        // Copia a palavra para o buffer
        strcpy(buffer + pos, word);
        pos += word_len;
        
        // Adiciona espaço ou quebra de linha
        if (rand() % 10 < 2) {  // 20% de chance de quebra de linha
            buffer[pos++] = '\n';
            if (rand() % 3 == 0) {  // 1/3 de chance de parágrafo duplo
                buffer[pos++] = '\n';
            }
        } else {
            buffer[pos++] = ' ';
        }
    }
    buffer[pos] = '\0';
}

void generate_file(const char *filename, long target_size, int use_words) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Erro ao criar arquivo");
        exit(EXIT_FAILURE);
    }
    
    char buffer[BUFFER_SIZE];
    long bytes_written = 0;
    
    printf("Gerando arquivo '%s' com %ld bytes...\n", filename, target_size);
    
    while (bytes_written < target_size) {
        size_t chunk_size = BUFFER_SIZE - 1;
        if (bytes_written + chunk_size > target_size) {
            chunk_size = target_size - bytes_written;
        }
        
        if (use_words) {
            generate_random_words(buffer, chunk_size);
        } else {
            generate_random_text(buffer, chunk_size);
        }
        
        size_t written = fwrite(buffer, 1, chunk_size, file);
        if (written != chunk_size) {
            perror("Erro ao escrever no arquivo");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        
        bytes_written += written;
        printf("\rProgresso: %.2f%%", (double)bytes_written / target_size * 100);
        fflush(stdout);
    }
    
    fclose(file);
    printf("\nArquivo gerado com sucesso: %s (%ld bytes)\n", filename, bytes_written);
}

void print_help() {
    printf("Gerador de Arquivos de Texto Aleatório\n");
    printf("Uso: ./file_generator <nome_arquivo> <tamanho> [opções]\n");
    printf("\nArgumentos:\n");
    printf("  nome_arquivo  Nome do arquivo a ser gerado\n");
    printf("  tamanho       Tamanho desejado em bytes (use sufixos K, M, G para KB, MB, GB)\n");
    printf("\nOpções:\n");
    printf("  -w           Usar palavras reais em vez de caracteres aleatórios\n");
    printf("  -h           Mostrar esta ajuda\n");
    printf("\nExemplos:\n");
    printf("  ./file_generator teste.txt 1M       # Gera arquivo de 1MB com texto aleatório\n");
    printf("  ./file_generator doc.txt 500K -w    # Gera arquivo de 500KB com palavras\n");
}

long parse_size(const char *size_str) {
    char *endptr;
    long size = strtol(size_str, &endptr, 10);
    
    if (*endptr != '\0') {
        switch (tolower(*endptr)) {
            case 'k': size *= 1024; break;
            case 'm': size *= 1024 * 1024; break;
            case 'g': size *= 1024 * 1024 * 1024; break;
            default:
                fprintf(stderr, "Sufixo de tamanho inválido: %c\n", *endptr);
                exit(EXIT_FAILURE);
        }
    }
    
    return size;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_help();
        return EXIT_FAILURE;
    }
    
    // Verifica pedido de ajuda
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help();
            return EXIT_SUCCESS;
        }
    }
    
    // Inicializa gerador de números aleatórios
    srand(time(NULL) ^ getpid());
    
    // Parse das opções
    int use_words = 0;
    int opt;
    while ((opt = getopt(argc, argv, "w")) != -1) {
        switch (opt) {
            case 'w': use_words = 1; break;
            default:
                print_help();
                return EXIT_FAILURE;
        }
    }
    
    // Verifica argumentos restantes
    if (optind + 2 > argc) {
        print_help();
        return EXIT_FAILURE;
    }
    
    const char *filename = argv[optind];
    long target_size = parse_size(argv[optind + 1]);
    
    if (target_size <= 0) {
        fprintf(stderr, "Tamanho inválido: %s\n", argv[optind + 1]);
        return EXIT_FAILURE;
    }
    
    generate_file(filename, target_size, use_words);
    return EXIT_SUCCESS;
}