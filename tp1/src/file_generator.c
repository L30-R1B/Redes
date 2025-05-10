#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char random_char() {
    return ' ' + (rand() % 95);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <MAX_N> <CHUNK_SIZE>\n", argv[0]);
        return 1;
    }

    int max_n = atoi(argv[1]);
    int chunk_size = atoi(argv[2]);

    if (max_n <= 0 || chunk_size <= 0) {
        fprintf(stderr, "Erro: os valores devem ser inteiros positivos.\n");
        return 1;
    }

    srand(time(NULL));

    for (int N = 1; N <= max_n; N++) {
        char filename[64];
        snprintf(filename, sizeof(filename), "file-%d.txt", N);

        FILE *fp = fopen(filename, "w");
        if (!fp) {
            perror("Erro ao criar arquivo");
            return 1;
        }

        for (int i = 0; i < N * chunk_size; i++) {
            fputc(random_char(), fp);
        }

        fclose(fp);
        printf("Arquivo %s gerado com %d bytes.\n", filename, N * chunk_size);
    }

    return 0;
}
