
#ifndef LOGGER_H
#define LOGGER_H

#include <time.h>

#define LOG_FILE "servidor.log"


void log_request(const char *client_ip, const char *request_line);

#endif 