#!/bin/bash

# Script: test_protocols.sh
# Descrição: Realiza testes comparativos organizados entre TCP e UDP
# Uso: ./test_protocols.sh <MAX_N> <CHUNK_SIZE> <NUM_TESTS>

# Verificar argumentos
if [ "$#" -ne 3 ]; then
    echo "Uso: $0 <MAX_N> <CHUNK_SIZE> <NUM_TESTS>"
    exit 1
fi

make

MAX_N=$1
CHUNK_SIZE=$2
NUM_TESTS=$3
SERVER_IP="127.0.0.1"
PORT="8080"
TEST_DIR="test_files"
RESULTS_DIR="test_results_$(date +%Y%m%d_%H%M%S)"
BIN_DIR="bin"
CLIENT_LOG_DIR="$RESULTS_DIR/client_logs"
SERVER_LOG_DIR="$RESULTS_DIR/server_logs"
RECEIVED_DIR="received_files"

# Criar estrutura de diretórios
echo "Criando estrutura de diretórios..."
mkdir -p $TEST_DIR
mkdir -p $RESULTS_DIR
mkdir -p $CLIENT_LOG_DIR
mkdir -p $SERVER_LOG_DIR
mkdir -p $RECEIVED_DIR

# Limpar arquivos anteriores
echo "Limpando arquivos de execuções anteriores..."
rm -f $TEST_DIR/file-*.txt
rm -f $RECEIVED_DIR/*
rm -f $BIN_DIR/server_*.log
rm -f $BIN_DIR/client_*.log

# Gerar arquivos de teste organizados
echo "Gerando arquivos de teste com padrão organizado..."
$BIN_DIR/file_generator $MAX_N $CHUNK_SIZE
mv file-*.txt $TEST_DIR/

# Iniciar servidores com logs organizados
echo "Iniciando servidores com logs separados..."
$BIN_DIR/server_tcp $TEST_DIR > $SERVER_LOG_DIR/server_tcp.log 2>&1 &
TCP_SERVER_PID=$!
$BIN_DIR/server_udp $TEST_DIR > $SERVER_LOG_DIR/server_udp.log 2>&1 &
UDP_SERVER_PID=$!

# Esperar inicialização dos servidores
echo "Aguardando inicialização dos servidores..."
sleep 2

# Função para executar testes de forma organizada
run_test() {
    local protocol=$1
    local client_bin="$BIN_DIR/client_$protocol"
    local result_file="$RESULTS_DIR/${protocol}_results.csv"
    local protocol_log_dir="$CLIENT_LOG_DIR/$protocol"
    
    mkdir -p $protocol_log_dir
    
    # Cabeçalho do arquivo de resultados
    echo "test_num,file_name,file_size,transfer_time,speed_kbs,status,packets_sent,packets_received,packet_loss" > $result_file
    
    for test_num in $(seq 1 $NUM_TESTS); do
        echo -e "\n=== Teste $test_num/$NUM_TESTS com $protocol ==="
        local test_log_dir="$protocol_log_dir/test_$test_num"
        mkdir -p $test_log_dir
        
        for file in $TEST_DIR/file-*.txt; do
            local file_name=$(basename $file)
            local file_size=$(stat -c%s "$file")
            local output_log="$test_log_dir/${file_name}.log"
            
            echo "  Processando $file_name ($file_size bytes)..."
            
            # Executar cliente e salvar saída completa
            $client_bin $SERVER_IP $PORT <<< "get $file_name" > "$output_log" 2>&1
            
            # Extrair métricas da saída
            local output=$(cat "$output_log")
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
            echo "$test_num,$file_name,$file_size,$transfer_time,$speed_kbs,$status,$packets_sent,$packets_received,$packet_loss" >> $result_file
        done
    done
}

# Executar testes organizados para ambos protocolos
echo -e "\nIniciando bateria de testes organizados..."
run_test "tcp"
run_test "udp"

# Encerrar servidores
echo -e "\nFinalizando servidores..."
kill $TCP_SERVER_PID
kill $UDP_SERVER_PID
wait

# Processar resultados e gerar relatórios organizados
echo -e "\nProcessando resultados e gerando relatórios..."

# Função para gerar relatório por protocolo
generate_protocol_report() {
    local protocol=$1
    local report_file="$RESULTS_DIR/${protocol}_report.txt"
    
    echo "=== Relatório Detalhado $protocol ===" > $report_file
    echo "Arquivos testados: $MAX_N (tamanhos de $CHUNK_SIZE até $((MAX_N * CHUNK_SIZE)) bytes" >> $report_file
    echo "Número de testes por arquivo: $NUM_TESTS" >> $report_file
    echo "------------------------------------" >> $report_file
    
    for file in $TEST_DIR/file-*.txt; do
        local file_name=$(basename $file)
        local file_size=$(stat -c%s "$file")
        local data=$(grep "$file_name" $RESULTS_DIR/${protocol}_results.csv)
        
        # Calcular estatísticas
        local avg_time=$(echo "$data" | awk -F, '{sum+=$4} END {printf "%.6f", sum/NR}')
        local avg_speed=$(echo "$data" | awk -F, '{sum+=$5} END {printf "%.2f", sum/NR}')
        local success_rate=$(echo "$data" | awk -F, 'BEGIN {count=0} $6=="OK" {count++} END {printf "%.2f", (count/NR)*100}')
        
        echo -e "\nArquivo: $file_name ($file_size bytes)" >> $report_file
        echo "  Tempo médio: $avg_time segundos" >> $report_file
        echo "  Velocidade média: $avg_speed KB/s" >> $report_file
        echo "  Taxa de sucesso: $success_rate%" >> $report_file
        
        if [ "$protocol" == "udp" ]; then
            local avg_loss=$(echo "$data" | awk -F, '{sum+=$9} END {printf "%.2f", sum/NR}')
            echo "  Perda média de pacotes: $avg_loss%" >> $report_file
        fi
    done
}

# Gerar relatórios individuais
generate_protocol_report "tcp"
generate_protocol_report "udp"

# Gerar relatório comparativo
echo -e "\nGerando relatório comparativo..."
cat > $RESULTS_DIR/comparative_report.txt <<EOL
=== Relatório Comparativo TCP vs UDP ===

Configuração do Teste:
- Arquivos testados: $MAX_N (de $CHUNK_SIZE até $((MAX_N * CHUNK_SIZE)) bytes)
- Número de testes por arquivo: $NUM_TESTS
- Diretório de resultados: $RESULTS_DIR

Resumo Estatístico:
EOL

# Adicionar tabela comparativa
printf "%-10s %-12s %-12s %-12s %-12s\n" "Tamanho" "TCP-Time" "UDP-Time" "TCP-Speed" "UDP-Speed" >> $RESULTS_DIR/comparative_report.txt
printf "%-10s %-12s %-12s %-12s %-12s\n" "(bytes)" "(seg)" "(seg)" "(KB/s)" "(KB/s)" >> $RESULTS_DIR/comparative_report.txt

for N in $(seq 1 $MAX_N); do
    file_name="file-${N}.txt"
    file_size=$((N * CHUNK_SIZE))
    
    tcp_data=$(grep "$file_name" $RESULTS_DIR/tcp_results.csv)
    udp_data=$(grep "$file_name" $RESULTS_DIR/udp_results.csv)
    
    tcp_avg_time=$(echo "$tcp_data" | awk -F, '{sum+=$4} END {printf "%.6f", sum/NR}')
    udp_avg_time=$(echo "$udp_data" | awk -F, '{sum+=$4} END {printf "%.6f", sum/NR}')
    tcp_avg_speed=$(echo "$tcp_data" | awk -F, '{sum+=$5} END {printf "%.2f", sum/NR}')
    udp_avg_speed=$(echo "$udp_data" | awk -F, '{sum+=$5} END {printf "%.2f", sum/NR}')
    
    printf "%-10d %-12s %-12s %-12s %-12s\n" $file_size $tcp_avg_time $udp_avg_time $tcp_avg_speed $udp_avg_speed >> $RESULTS_DIR/comparative_report.txt
done

# Limpeza final organizada
echo -e "\nRealizando limpeza final..."
rm -f $TEST_DIR/file-*.txt
rm -f $RECEIVED_DIR/*
rmdir $RECEIVED_DIR 2>/dev/null

mv server_udp.log $RESULTS_DIR/server_logs/
mv server_tcp.log $RESULTS_DIR/server_logs/

mv client_udp.log $RESULTS_DIR/client_logs/
mv client_tcp.log $RESULTS_DIR/client_logs/

rm file-*

echo -e "\n=== Testes concluídos com sucesso ==="
echo "Resultados disponíveis em: $RESULTS_DIR"
echo "Estrutura de diretórios:"
