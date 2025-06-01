


CC = gcc
CFLAGS = -Wall -Wextra -g -I./common 
LDFLAGS_PTHREAD = -pthread




                
                
COMMON_DIR = ./common
SERVERS_DIR = ./servers


COMMON_OBJS = $(COMMON_DIR)/socket_utils.o $(COMMON_DIR)/logger.o $(COMMON_DIR)/mime_types.o $(COMMON_DIR)/http_handler.o


TARGET_ITERATIVO = servidor_iterativo
TARGET_FORK = servidor_fork
TARGET_THREAD_PER_CLIENTE = servidor_thread_per_cliente
TARGET_THREAD_POOL = servidor_thread_pool
TARGET_SELECT = servidor_select


TARGETS = $(TARGET_ITERATIVO) $(TARGET_FORK) $(TARGET_THREAD_PER_CLIENTE) $(TARGET_THREAD_POOL) $(TARGET_SELECT)

.PHONY: all clean

all: $(TARGETS)







$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.c $(COMMON_DIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $@







$(TARGET_ITERATIVO): $(SERVERS_DIR)/servidor_iterativo.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TARGET_FORK): $(SERVERS_DIR)/servidor_fork.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TARGET_THREAD_PER_CLIENTE): $(SERVERS_DIR)/servidor_thread_per_cliente.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS_PTHREAD)

$(TARGET_THREAD_POOL): $(SERVERS_DIR)/servidor_thread_pool.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS_PTHREAD)

$(TARGET_SELECT): $(SERVERS_DIR)/servidor_select.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@




$(SERVERS_DIR)/%.o: $(SERVERS_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(TARGETS) $(SERVERS_DIR)/*.o $(COMMON_DIR)/*.o
	rm -f servidor.log 
	rm -rf test_results 



























