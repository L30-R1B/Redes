#!/bin/bash

# Script: test_protocols.sh
# Descrição: Realiza testes em massa comparando desempenho de TCP e UDP
# Uso: ./test_protocols.sh <MAX_N> <CHUNK_SIZE> <NUM_TESTS>

# Verificar argumentos
if [ "$#" -ne 3 ]; then
    echo "Uso: $0 <MAX_N> <CHUNK_SIZE> <NUM_TESTS>"
    exit 1
fi

MAX_N=$1
CHUNK_SIZE=$2
NUM_TESTS=$3
SERVER_IP="127.0.0.1"
PORT="8080"
TEST_DIR="test_files"
RESULTS_DIR="test_results"
BIN_DIR="bin"

# Criar diretórios necessários
mkdir -p $TEST_DIR
mkdir -p $RESULTS_DIR

# Gerar arquivos de teste
echo "Gerando arquivos de teste..."
$BIN_DIR/file_generator $MAX_N $CHUNK_SIZE
mv file-*.txt $TEST_DIR/

# Iniciar servidores em segundo plano
echo "Iniciando servidores..."
$BIN_DIR/server_tcp $TEST_DIR > $RESULTS_DIR/server_tcp.log 2>&1 &
TCP_SERVER_PID=$!
$BIN_DIR/server_udp $TEST_DIR > $RESULTS_DIR/server_udp.log 2>&1 &
UDP_SERVER_PID=$!

# Esperar servidores iniciarem
sleep 2

# Função para testar um protocolo
test_protocol() {
    local protocol=$1
    local client_bin="$BIN_DIR/client_$protocol"
    local result_file="$RESULTS_DIR/${protocol}_results.csv"
    
    # Cabeçalho do arquivo de resultados
    echo "file_name,file_size,transfer_time,speed_kbs,status,packets_sent,packets_received,packet_loss" > $result_file
    
    for test_num in $(seq 1 $NUM_TESTS); do
        echo "Executando teste $test_num/$NUM_TESTS com $protocol..."
        
        for file in $TEST_DIR/file-*.txt; do
            local file_name=$(basename $file)
            local file_size=$(stat -c%s "$file")
            
            # Executar cliente e capturar saída
            local output=$($client_bin $SERVER_IP $PORT <<< "get $file_name" 2>&1)
            
            # Extrair métricas da saída
            local transfer_time=$(echo "$output" | grep "Tempo:" | awk '{print $2}')
            local speed_kbs=$(echo "$output" | grep "Velocidade:" | awk '{print $2}')
            local status=$(echo "$output" | grep "Status:" | awk '{print $2}')
            
            # Valores específicos para UDP
            local packets_sent="N/A"
            local packets_received="N/A"
            local packet_loss="N/A"
            
            if [ "$protocol" == "udp" ]; then
                packets_sent=$(echo "$output" | grep "Pacotes esperados:" | awk '{print $3}')
                packets_received=$(echo "$output" | grep "Pacotes recebidos:" | awk '{print $3}')
                packet_loss=$(echo "$output" | grep "Pacotes perdidos:" | awk '{print $3}')
            fi
            
            # Registrar resultados
            echo "$file_name,$file_size,$transfer_time,$speed_kbs,$status,$packets_sent,$packets_received,$packet_loss" >> $result_file
        done
    done
}

# Executar testes para ambos os protocolos
test_protocol "tcp"
test_protocol "udp"

# Encerrar servidores
echo "Encerrando servidores..."
kill $TCP_SERVER_PID
kill $UDP_SERVER_PID

# Processar resultados e gerar relatório
echo "Processando resultados..."

# Gerar relatório comparativo
echo "Gerando relatório comparativo..."
cat > $RESULTS_DIR/comparative_report.txt <<EOL
=== Relatório Comparativo TCP vs UDP ===

Arquivos testados: $MAX_N (de $CHUNK_SIZE até $((MAX_N * CHUNK_SIZE)) bytes)
Número de testes por arquivo: $NUM_TESTS

Métricas Analisadas:
1. Tempo médio de transferência
2. Velocidade média de transferência
3. Taxa de sucesso (MD5 válido)
4. Perda de pacotes (apenas UDP)

EOL

# Adicionar análise para cada arquivo
for file in $TEST_DIR/file-*.txt; do
    file_name=$(basename $file)
    file_size=$(stat -c%s "$file")
    
    # Calcular médias para TCP
    tcp_data=$(grep "$file_name" $RESULTS_DIR/tcp_results.csv)
    tcp_avg_time=$(echo "$tcp_data" | awk -F, '{sum+=$3} END {print sum/NR}')
    tcp_avg_speed=$(echo "$tcp_data" | awk -F, '{sum+=$4} END {print sum/NR}')
    tcp_success_rate=$(echo "$tcp_data" | awk -F, 'BEGIN {count=0} $5=="OK" {count++} END {print count/NR*100}')
    
    # Calcular médias para UDP
    udp_data=$(grep "$file_name" $RESULTS_DIR/udp_results.csv)
    udp_avg_time=$(echo "$udp_data" | awk -F, '{sum+=$3} END {print sum/NR}')
    udp_avg_speed=$(echo "$udp_data" | awk -F, '{sum+=$4} END {print sum/NR}')
    udp_success_rate=$(echo "$udp_data" | awk -F, 'BEGIN {count=0} $5=="OK" {count++} END {print count/NR*100}')
    udp_avg_loss=$(echo "$udp_data" | awk -F, '{sum+=$8} END {print sum/NR}')
    
    # Adicionar ao relatório
    cat >> $RESULTS_DIR/comparative_report.txt <<EOL
=== Arquivo: $file_name ($file_size bytes) ===

TCP:
- Tempo médio: $(printf "%.6f" $tcp_avg_time) segundos
- Velocidade média: $(printf "%.2f" $tcp_avg_speed) KB/s
- Taxa de sucesso: $(printf "%.2f" $tcp_success_rate)%

UDP:
- Tempo médio: $(printf "%.6f" $udp_avg_time) segundos
- Velocidade média: $(printf "%.2f" $udp_avg_speed) KB/s
- Taxa de sucesso: $(printf "%.2f" $udp_success_rate)%
- Perda média de pacotes: $(printf "%.2f" $udp_avg_loss)%

EOL
done

echo "Testes concluídos! Resultados salvos em $RESULTS_DIR/"

echo "Limpando arquivos de teste..."
rm -f $TEST_DIR/file-*.txt