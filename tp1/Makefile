# Makefile para o projeto de redes

# Compilador
CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lssl -lcrypto

# Diretórios
BIN_DIR = bin
SRC_DIR = src
TEST_DIR = test

# Arquivos
SERVER_TCP_SRC = $(SRC_DIR)/server_tcp.c
CLIENT_TCP_SRC = $(SRC_DIR)/client_tcp.c
SERVER_UDP_SRC = $(SRC_DIR)/server_udp.c
CLIENT_UDP_SRC = $(SRC_DIR)/client_udp.c
FILE_GEN_SRC = $(SRC_DIR)/file_generator.c

# Executáveis
SERVER_TCP = $(BIN_DIR)/server_tcp
CLIENT_TCP = $(BIN_DIR)/client_tcp
SERVER_UDP = $(BIN_DIR)/server_udp
CLIENT_UDP = $(BIN_DIR)/client_udp
FILE_GEN = $(BIN_DIR)/file_generator

# Regra padrão
all: directories $(SERVER_TCP) $(CLIENT_TCP) $(SERVER_UDP) $(CLIENT_UDP) $(FILE_GEN)

# Criar diretório bin se não existir
directories:
	mkdir -p $(BIN_DIR)

# Compilar servidor TCP
$(SERVER_TCP): $(SERVER_TCP_SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Compilar cliente TCP
$(CLIENT_TCP): $(CLIENT_TCP_SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Compilar servidor UDP
$(SERVER_UDP): $(SERVER_UDP_SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Compilar cliente UDP
$(CLIENT_UDP): $(CLIENT_UDP_SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Compilar gerador de arquivos
$(FILE_GEN): $(FILE_GEN_SRC)
	$(CC) $(CFLAGS) $< -o $@

# Limpar
clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean directories