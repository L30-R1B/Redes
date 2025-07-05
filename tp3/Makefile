# Makefile para compilar o cliente e o servidor do protocolo Stop-and-Wait

# Compilador e flags
CC = gcc
# Flags: -Wall (todos os avisos), -std=c11 (padrão C11), -g (símbolos de depuração), -O2 (otimização)
CFLAGS = -Wall -std=c11 -g -O2

# Bibliotecas de ligação
# -lrt é necessário para clock_gettime no cliente
LDLIBS = -lrt

# Alvos executáveis
TARGETS = client server

# Alvo padrão: compila todos os executáveis
#.PHONY garante que 'all' e 'clean' sejam executados mesmo que existam arquivos com esses nomes.
.PHONY: all clean

all: $(TARGETS)

# Regra de ligação para o cliente (com a biblioteca -lrt)
client: client.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Regra de ligação para o servidor
server: server.o
	$(CC) $(CFLAGS) -o $@ $^

# Regra de padrão para compilar arquivos .c em .o
# Qualquer arquivo .o depende do seu .c correspondente e de protocol.h
# $< é a primeira dependência (o arquivo .c)
%.o: %.c protocol.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Regra para limpar os arquivos gerados
clean:
	rm -f $(TARGETS) *.o received_file.txt original_file.txt