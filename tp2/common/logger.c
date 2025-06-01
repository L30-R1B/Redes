

#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h> 


static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_request(const char *client_ip, const char *request_line) {
    FILE *log_file;
    time_t now;
    char time_buffer[100];

    time(&now);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", localtime(&now));

    pthread_mutex_lock(&log_mutex); 

    log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        perror("Erro ao abrir arquivo de log");
        pthread_mutex_unlock(&log_mutex); 
        return;
    }

    fprintf(log_file, "%s - [%s] - \"%s\"\n", client_ip, time_buffer, request_line);
    fflush(log_file); 
    fclose(log_file);

    pthread_mutex_unlock(&log_mutex); 
}