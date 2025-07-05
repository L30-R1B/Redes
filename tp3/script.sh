#!/bin/bash

# ==============================================================================
# Script de Teste Avançado para o Protocolo Stop-and-Wait
#
# Funcionalidades:
# - Testa uma faixa de taxas de perda (0-100%).
# - Cria um diretório com timestamp para cada execução.
# - Salva um log detalhado de toda a saída.
# - Gera um arquivo CSV com os resultados para fácil análise.
# ==============================================================================

# --- Configurações ---
PORT=8080
TEST_FILE="original_file.txt"
FILE_SIZE_KB=100
SERVER_PID=0

# Cores para a saída no console
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # Sem Cor

# --- Criação do Diretório de Resultados ---
TIMESTAMP=$(date +'%Y-%m-%d_%H-%M-%S')
RESULTS_BASE_DIR="test_results"
RESULTS_DIR="${RESULTS_BASE_DIR}/${TIMESTAMP}"
mkdir -p "$RESULTS_DIR"

LOG_FILE="${RESULTS_DIR}/test_log.txt"
CSV_FILE="${RESULTS_DIR}/results.csv"

# --- Função de Limpeza ---
# Garante que o servidor seja finalizado e os arquivos temporários removidos
cleanup() {
    echo "" | tee -a "$LOG_FILE"
    echo "Executando limpeza..." | tee -a "$LOG_FILE"
    if [ $SERVER_PID -ne 0 ]; then
        kill $SERVER_PID 2>/dev/null
        wait $SERVER_PID 2>/dev/null
    fi
    rm -f "$TEST_FILE" "received_file.txt"
    make clean > /dev/null
    echo "Limpeza concluída." | tee -a "$LOG_FILE"
    echo -e "\n${GREEN}Resultados salvos em: ${YELLOW}${RESULTS_DIR}${NC}"
}

# Configura o trap para chamar a função cleanup na saída (EXIT)
trap cleanup EXIT

# --- Início do Script ---
echo "Iniciando bateria de testes..."
echo "Resultados serão salvos em: $RESULTS_DIR"
echo "------------------------------------------------------"

# Compila o projeto
echo "Compilando o projeto..." | tee -a "$LOG_FILE"
(make clean && make) >> "$LOG_FILE" 2>&1
if [ $? -ne 0 ]; then
    echo -e "${RED}Falha na compilação. Verifique o log em ${LOG_FILE}. Abortando.${NC}"
    exit 1
fi

# Cria o arquivo de teste
echo "Criando arquivo de teste de ${FILE_SIZE_KB}KB..." | tee -a "$LOG_FILE"
dd if=/dev/urandom of=$TEST_FILE bs=1024 count=$FILE_SIZE_KB >> "$LOG_FILE" 2>&1

# Cria o cabeçalho do arquivo CSV
echo "TaxaDePerda(%),Status,TempoTotal(s),Retransmissoes,TaxaDeTransferencia(KB/s)" > "$CSV_FILE"

# --- Loop Principal de Testes ---
# Testa de 0% a 100% com passo de 5
for loss in $(seq 0 5 100); do
    
    # Redireciona toda a saída deste bloco para o arquivo de log
    {
        echo ""
        echo "======================================================"
        echo "Iniciando teste com taxa de perda de ${loss}%"
        echo "======================================================"

        # Inicia o servidor em background
        ./server -p $PORT -l $loss &
        SERVER_PID=$!
        sleep 1 # Dá um tempo para o servidor iniciar

        # Executa o cliente e captura sua saída para análise
        CLIENT_OUTPUT=$(./client -h 127.0.0.1 -p $PORT -f $TEST_FILE)
        
        # Imprime a saída do cliente no log
        echo "$CLIENT_OUTPUT"

        # Mata o processo do servidor
        kill $SERVER_PID
        wait $SERVER_PID 2>/dev/null
        SERVER_PID=0
        
        RECEIVED_FILE="received_file.txt"
        STATUS="FALHOU"
        COLOR=$RED

        # Verifica o resultado
        if [ -f "$RECEIVED_FILE" ] && diff "$TEST_FILE" "$RECEIVED_FILE" > /dev/null; then
            STATUS="SUCESSO"
            COLOR=$GREEN
        fi
        
        echo -e "\nResultado do Teste: ${COLOR}${STATUS}${NC}"

        # --- Extração de Dados para o CSV ---
        TIME_TAKEN=$(echo "$CLIENT_OUTPUT" | grep "Tempo total" | awk '{print $4}')
        RETRANSMISSIONS=$(echo "$CLIENT_OUTPUT" | grep "Total de retransmissões" | awk '{print $4}')
        THROUGHPUT=$(echo "$CLIENT_OUTPUT" | grep "Taxa de transferência" | awk '{print $4}')
        
        # Garante que os valores não fiquem vazios em caso de falha
        TIME_TAKEN=${TIME_TAKEN:-"N/A"}
        RETRANSMISSIONS=${RETRANSMISSIONS:-"N/A"}
        THROUGHPUT=${THROUGHPUT:-"N/A"}

        # Adiciona a linha de dados ao arquivo CSV
        echo "$loss,$STATUS,$TIME_TAKEN,$RETRANSMISSIONS,$THROUGHPUT" >> "$CSV_FILE"

        rm -f "$RECEIVED_FILE"
    
    } >> "$LOG_FILE" 2>&1 # Fim do redirecionamento para o log

    # Imprime um status rápido no console
    echo -e "Teste com ${loss}% de perda... Resultado: ${COLOR}${STATUS}${NC}"

done

echo "------------------------------------------------------"
echo "Todos os testes foram concluídos."
