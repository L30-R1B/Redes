
#include "../common/socket_utils.h"
#include "../common/http_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define NUM_THREADS 4     
#define TAMANHO_FILA 100  


typedef struct {
    int socket_cliente;
    struct sockaddr_storage client_addr_storage;
} task_t;


task_t fila_tarefas[TAMANHO_FILA];
int contador_fila = 0;
int inicio_fila = 0;
int fim_fila = 0;

pthread_mutex_t mutex_fila = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_fila_nao_vazia = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_fila_nao_cheia = PTHREAD_COND_INITIALIZER;


void enfileirar_tarefa(task_t tarefa) {
    pthread_mutex_lock(&mutex_fila);

    while (contador_fila == TAMANHO_FILA) {
        
        printf("Fila de tarefas cheia. Produtor aguardando...\n");
        pthread_cond_wait(&cond_fila_nao_cheia, &mutex_fila);
    }

    fila_tarefas[fim_fila] = tarefa;
    fim_fila = (fim_fila + 1) % TAMANHO_FILA;
    contador_fila++;

    
    pthread_cond_signal(&cond_fila_nao_vazia);
    pthread_mutex_unlock(&mutex_fila);
}


task_t desenfileirar_tarefa() {
    task_t tarefa;
    pthread_mutex_lock(&mutex_fila);

    while (contador_fila == 0) {
        
        printf("Fila de tarefas vazia. Consumidor (TID: %lu) aguardando...\n", (unsigned long)pthread_self());
        pthread_cond_wait(&cond_fila_nao_vazia, &mutex_fila);
    }

    tarefa = fila_tarefas[inicio_fila];
    inicio_fila = (inicio_fila + 1) % TAMANHO_FILA;
    contador_fila--;

    
    pthread_cond_signal(&cond_fila_nao_cheia);
    pthread_mutex_unlock(&mutex_fila);

    return tarefa;
}


void *worker_thread_func(void *arg) {
    (void)arg; 
    printf("Thread trabalhadora (TID: %lu) iniciada.\n", (unsigned long)pthread_self());
    while (1) {
        task_t tarefa_atual = desenfileirar_tarefa();
        
        char *ip_cliente_str = obter_ip_cliente(&(tarefa_atual.client_addr_storage));
        printf("Thread (TID: %lu) tratando cliente %s (socket: %d)\n", 
               (unsigned long)pthread_self(), 
               ip_cliente_str ? ip_cliente_str : "IP desconhecido",
               tarefa_atual.socket_cliente);
        if(ip_cliente_str) free(ip_cliente_str);

        tratar_conexao_http(tarefa_atual.socket_cliente, &(tarefa_atual.client_addr_storage));
        
    }
    return NULL; 
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int porta = atoi(argv[1]);
    int socket_escuta;
    pthread_t threads_trabalhadoras[NUM_THREADS];

    socket_escuta = criar_socket_escuta(porta);
    if (socket_escuta == -1) {
        exit(EXIT_FAILURE);
    }

    
    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_create(&threads_trabalhadoras[i], NULL, worker_thread_func, NULL) != 0) {
            perror("Erro ao criar thread trabalhadora");
            
            
            exit(EXIT_FAILURE); 
        }
        
                                                  
                                                  
    }
     for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_detach(threads_trabalhadoras[i]);
    }


    printf("Servidor com Pool de Threads (%d threads) iniciado. Aguardando conexões...\n", NUM_THREADS);

    while (1) {
        task_t nova_tarefa;
        socklen_t tamanho_endereco_cliente = sizeof(nova_tarefa.client_addr_storage);

        nova_tarefa.socket_cliente = accept(socket_escuta, 
                                            (struct sockaddr *)&(nova_tarefa.client_addr_storage), 
                                            &tamanho_endereco_cliente);
        
        if (nova_tarefa.socket_cliente < 0) {
            perror("Erro no accept");
            continue; 
        }
        
        char *ip_cliente_str_main = obter_ip_cliente(&(nova_tarefa.client_addr_storage));
        printf("Conexão aceita de %s. Enfileirando tarefa...\n", ip_cliente_str_main ? ip_cliente_str_main : "IP desconhecido");
        if(ip_cliente_str_main) free(ip_cliente_str_main);

        enfileirar_tarefa(nova_tarefa);
    }

    
    close(socket_escuta);
    
    
    
    
    

    return 0;
}
