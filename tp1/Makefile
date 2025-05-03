CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lssl -lcrypto

# Diretórios
BIN_DIR = bin
OBJ_DIR = obj
SRC_DIR = .

# Criar diretórios se não existirem
$(shell mkdir -p $(BIN_DIR) $(OBJ_DIR))

# Lista de alvos (mantive apenas TCP)
TARGETS = $(BIN_DIR)/server_tcp $(BIN_DIR)/client_tcp

# Regra padrão
all: $(TARGETS)

# Objetos
$(OBJ_DIR)/server_tcp.o: $(SRC_DIR)/server_tcp.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/client_tcp.o: $(SRC_DIR)/client_tcp.c
	$(CC) $(CFLAGS) -c $< -o $@

# Executáveis
$(BIN_DIR)/server_tcp: $(OBJ_DIR)/server_tcp.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/client_tcp: $(OBJ_DIR)/client_tcp.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Limpeza
clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)

.PHONY: all clean