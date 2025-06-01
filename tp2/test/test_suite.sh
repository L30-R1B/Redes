#!/bin/bash

# Script para testar os servidores web, coletar métricas, gerar gráficos e estatísticas.

# --- Configurações ---
SERVER_HOST="localhost"
PORTA_TESTE=8088

ARQUIVOS_TESTE=("/image.png" "/image2.png" "/image3.png" "/image4.png") # Adicione mais arquivos se desejar
CONCORRENCIAS=(1 5 10 20 50)                # Ajuste conforme necessário
NUM_REQUISICOES=1000                        # Ajuste para testes mais longos ou mais curtos

# Diretório base para todos os resultados
RESULTS_BASE_DIR="test_results"
# Diretório para a execução atual do teste (com timestamp)
CURRENT_EXEC_DIR="${RESULTS_BASE_DIR}/$(date +"%Y-%m-%d_%H-%M-%S")"

# Caminhos dentro do diretório da execução atual
RAW_DATA_DIR="${CURRENT_EXEC_DIR}/raw_data"
GRAPHS_DIR="${CURRENT_EXEC_DIR}/graphs"
SUMMARY_CSV_FILE="${CURRENT_EXEC_DIR}/summary_all_tests.csv"
STATS_CSV_FILE="${CURRENT_EXEC_DIR}/aggregated_stats.csv"
LOG_AB_TSV_PREFIX="ab_data" # Prefixo para arquivos .tsv do ab -g

BASE_DIR=$(dirname "$0")/..
SERVER_EXEC_DIR="${BASE_DIR}" # Assumindo que o Makefile os coloca na raiz do projeto

SERVIDORES=(
    "servidor_iterativo"
    "servidor_fork"
    "servidor_thread_per_cliente"
    "servidor_thread_pool"
    "servidor_select"
)


# --- Funções Auxiliares ---
timestamp() {
    date +"%Y-%m-%d %H:%M:%S"
}

check_dependencies() {
    local missing_deps=0
    if ! command -v ab &>/dev/null; then
        echo "ERRO: Apache Benchmark (ab) não encontrado. Por favor, instale-o (ex: sudo apt-get install apache2-utils)."
        missing_deps=1
    fi
    if ! command -v lsof &>/dev/null; then
        echo "ERRO: lsof não encontrado. Por favor, instale-o (ex: sudo apt-get install lsof)."
        missing_deps=1
    fi
    if [ "$missing_deps" -ne 0 ]; then
        exit 1
    fi
}

setup_directories() {
    mkdir -p "${CURRENT_EXEC_DIR}"
    mkdir -p "${RAW_DATA_DIR}"
    # mkdir -p "${GRAPHS_DIR}" # Diretórios de gráficos não são mais necessários

    # Limpa/Cria o arquivo de resultados CSV
    echo "Resultados Detalhados dos Testes de Desempenho dos Servidores Web" > "$SUMMARY_CSV_FILE"
    echo "Timestamp da Execução: $(timestamp)" >> "$SUMMARY_CSV_FILE"
    echo "-------------------------------------------------------" >> "$SUMMARY_CSV_FILE"
    echo "Servidor,Arquivo,Concorrencia,ReqsPorSegundo,TempoPorReq_ms,Falhas" >> "$SUMMARY_CSV_FILE"
}

ensure_port_is_free() {
    local port_to_check="$1"
    local pids
    # -sTCP:LISTEN garante que pegamos apenas PIDs de processos ouvindo ativamente na porta TCP
    echo "    Verificando se a porta ${port_to_check} está livre..."
    pids=$(lsof -t -i :"${port_to_check}" -sTCP:LISTEN 2>/dev/null)

    if [ -n "$pids" ]; then
        echo "    AVISO: A porta ${port_to_check} está em uso pelos seguintes PIDs: $(echo "$pids" | tr '\n' ' ')."
        echo "    Tentando encerrar esses processos..."
        # Tenta encerrar de forma amigável primeiro
        for pid_to_kill in $pids; do
            if ps -p "$pid_to_kill" > /dev/null; then # Verifica se o PID ainda existe
                 kill "$pid_to_kill" 2>/dev/null
            fi
        done
        sleep 0.5 # Dá um tempo para o encerramento amigável

        # Reverifica e usa kill -9 se necessário
        pids=$(lsof -t -i :"${port_to_check}" -sTCP:LISTEN 2>/dev/null)
        if [ -n "$pids" ]; then
            echo "    Processos ainda ativos na porta ${port_to_check}. Forçando (kill -9)..."
            for pid_to_kill in $pids; do
                if ps -p "$pid_to_kill" > /dev/null; then
                    kill -9 "$pid_to_kill" 2>/dev/null
                fi
            done
            sleep 0.5 # Dá um tempo para o encerramento forçado
        fi

        # Verificação final
        pids=$(lsof -t -i :"${port_to_check}" -sTCP:LISTEN 2>/dev/null)
        if [ -n "$pids" ]; then
            echo "    ERRO CRÍTICO: Não foi possível liberar a porta ${port_to_check}. PIDs ainda ativos: $(echo "$pids" | tr '\n' ' ')."
            echo "    Por favor, libere a porta manualmente (ex: 'sudo kill -9 $pids') e tente novamente."
            return 1 # Indica falha
        else
            echo "    Porta ${port_to_check} liberada com sucesso."
        fi
    else
        echo "    Porta ${port_to_check} está livre."
    fi
    return 0 # Indica sucesso
}

# --- Script Principal ---
check_dependencies
setup_directories

echo "Iniciando suíte de testes. Resultados serão salvos em: ${CURRENT_EXEC_DIR}"
echo "Pressione Ctrl+C para interromper a qualquer momento."

# --- Loop Principal de Testes ---
for servidor_nome in "${SERVIDORES[@]}"; do
    servidor_exec="${SERVER_EXEC_DIR}/${servidor_nome}"
    SERVER_RAW_DATA_DIR="${RAW_DATA_DIR}/${servidor_nome}"
    mkdir -p "${SERVER_RAW_DATA_DIR}"

    if [ ! -x "$servidor_exec" ]; then
        echo "AVISO: Executável do servidor ${servidor_exec} não encontrado ou não é executável. Pulando..."
        # Adicionar uma linha ao CSV para indicar que este servidor foi pulado
        for arquivo_req_skip in "${ARQUIVOS_TESTE[@]}"; do
            for concorrencia_nivel_skip in "${CONCORRENCIAS[@]}"; do
                 echo "${servidor_nome},${arquivo_req_skip},${concorrencia_nivel_skip},NAO_EXECUTADO,NAO_EXECUTADO,NAO_EXECUTADO" >> "$SUMMARY_CSV_FILE"
            done
        done
        continue
    fi

    echo ""
    echo ">>> Iniciando testes para o servidor: ${servidor_nome} <<<"

    for arquivo_req in "${ARQUIVOS_TESTE[@]}"; do
        for concorrencia_nivel in "${CONCORRENCIAS[@]}"; do
            echo "  Testando ${servidor_nome} com ${arquivo_req}, Concorrência: ${concorrencia_nivel}, Requisições: ${NUM_REQUISICOES}"

            echo "    Iniciando ${servidor_nome} na porta ${PORTA_TESTE}..."
            "${servidor_exec}" "${PORTA_TESTE}" &
            SERVER_PID=$!
            sleep 2 # Aguardar o servidor iniciar

            if ! ps -p $SERVER_PID > /dev/null; then
                echo "    ERRO: Servidor ${servidor_nome} não iniciou corretamente (PID: ${SERVER_PID})."
                kill -9 $SERVER_PID 2>/dev/null
                echo "${servidor_nome},${arquivo_req},${concorrencia_nivel},ERRO_INICIO,ERRO_INICIO,ERRO_INICIO" >> "$SUMMARY_CSV_FILE"
                continue
            fi

            AB_TSV_FILE="${SERVER_RAW_DATA_DIR}/${LOG_AB_TSV_PREFIX}_${servidor_nome}_c${concorrencia_nivel}_f${arquivo_req//\//_}.tsv"
            TARGET_URL="http://${SERVER_HOST}:${PORTA_TESTE}${arquivo_req}"
            
            echo "    Executando ab (output em ${AB_TSV_FILE})..."
            AB_OUTPUT=$(ab -n "${NUM_REQUISICOES}" -c "${concorrencia_nivel}" -g "${AB_TSV_FILE}" "${TARGET_URL}" 2>&1)
            AB_EXIT_CODE=$?
            
            rps="ERRO_AB"; tpr="ERRO_AB"; falhas="ERRO_AB" # Valores padrão em caso de erro

            if [ $AB_EXIT_CODE -ne 0 ]; then
                echo "    ERRO: ab falhou para ${servidor_nome} com ${arquivo_req}, Concorrência: ${concorrencia_nivel}."
                echo "    Saída do ab:"
                echo "${AB_OUTPUT}" | sed 's/^/    | /' # Indenta a saída do ab
                falhas_raw=$(echo "${AB_OUTPUT}" | grep "Failed requests:" | awk '{print $3}')
                if [ -n "$falhas_raw" ]; then falhas=$falhas_raw; fi
            else
                rps=$(echo "${AB_OUTPUT}" | grep "Requests per second:" | awk '{print $4}')
                tpr=$(echo "${AB_OUTPUT}" | grep "Time per request:" | grep "across all concurrent requests" | awk '{print $4}')
                falhas=$(echo "${AB_OUTPUT}" | grep "Failed requests:" | awk '{print $3}')
                
                if [ -z "$rps" ]; then rps="N/A"; fi
                if [ -z "$tpr" ]; then tpr="N/A"; fi
                if [ -z "$falhas" ]; then falhas="0"; fi # Assumir 0 se não encontrado e ab teve sucesso

                echo "    Resultados: RPS=${rps}, TPR=${tpr} ms, Falhas=${falhas}"
            fi

            echo "${servidor_nome},${arquivo_req},${concorrencia_nivel},${rps},${tpr},${falhas}" >> "$SUMMARY_CSV_FILE"
            
            echo "    Parando ${servidor_nome} (PID ${SERVER_PID})..."
            kill $SERVER_PID
            wait $SERVER_PID 2>/dev/null
            sleep 0.5
            if ps -p $SERVER_PID > /dev/null; then
                echo "    Servidor ${servidor_nome} (PID ${SERVER_PID}) não parou, forçando..."
                kill -9 $SERVER_PID 2>/dev/null
            fi
            echo "    Servidor ${servidor_nome} parado."
            echo "    ----------------------------------------"
            sleep 1
        done 
    done 
    echo "" 
done 

echo ">>> Testes concluídos. <<<"

# --- Geração de Estatísticas ---
echo ""
echo ">>> Gerando estatísticas... <<<"

# Calcular e salvar estatísticas agregadas
echo "  Calculando estatísticas agregadas..."
awk '
BEGIN { 
    FS=","; OFS=","; 
    print "Servidor,Arquivo,AvgRPS,MinRPS,MaxRPS,AvgTPR_ms,MinTPR_ms,MaxTPR_ms,TotalFalhas" 
}
NR > 4 { # Pular cabeçalhos do CSV de entrada (3 linhas de meta + 1 de header)
    server_file_key = $1 FS $2
    
    if ($4 != "ERRO_AB" && $4 != "N/A" && $4 != "ERRO_INICIO" && $4 != "NAO_EXECUTADO") {
        sum_rps[server_file_key] += $4
        count_rps[server_file_key]++
        if (min_rps[server_file_key] == "" || $4 < min_rps[server_file_key]) min_rps[server_file_key] = $4
        if (max_rps[server_file_key] == "" || $4 > max_rps[server_file_key]) max_rps[server_file_key] = $4
    }
    if ($5 != "ERRO_AB" && $5 != "N/A" && $5 != "ERRO_INICIO" && $5 != "NAO_EXECUTADO") {
        sum_tpr[server_file_key] += $5
        count_tpr[server_file_key]++
        if (min_tpr[server_file_key] == "" || $5 < min_tpr[server_file_key]) min_tpr[server_file_key] = $5
        if (max_tpr[server_file_key] == "" || $5 > max_tpr[server_file_key]) max_tpr[server_file_key] = $5
    }
    if ($6 != "ERRO_AB" && $6 != "N/A" && $6 != "ERRO_INICIO" && $6 != "NAO_EXECUTADO") {
        sum_falhas[server_file_key] += $6
    } else if ($6 == "ERRO_AB" || $6 == "ERRO_INICIO" || $6 == "NAO_EXECUTADO") {
        # Se houve erro, não podemos somar, mas podemos querer registrar que houve falha
        # Para simplificar, apenas não somamos. Poderia ser um contador de "testes com erro".
    }
}
END {
    for (key in sum_rps) { # Iterar sobre chaves onde houve dados de RPS válidos
        split(key, parts, FS)
        server = parts[1]
        file = parts[2]
        
        avg_rps = (count_rps[key] > 0) ? sprintf("%.2f", sum_rps[key] / count_rps[key]) : "N/A"
        s_min_rps = (min_rps[key] != "") ? sprintf("%.2f", min_rps[key]) : "N/A"
        s_max_rps = (max_rps[key] != "") ? sprintf("%.2f", max_rps[key]) : "N/A"
        
        avg_tpr = (count_tpr[key] > 0) ? sprintf("%.3f", sum_tpr[key] / count_tpr[key]) : "N/A"
        s_min_tpr = (min_tpr[key] != "") ? sprintf("%.3f", min_tpr[key]) : "N/A"
        s_max_tpr = (max_tpr[key] != "") ? sprintf("%.3f", max_tpr[key]) : "N/A"

        s_falhas = (sum_falhas[key] != "") ? sum_falhas[key] : "0"
        if (count_rps[key] == 0 && count_tpr[key] == 0) { # Se não houve dados válidos para este server/file
             s_falhas = "N/A" # Ou o número de concorrências testadas se quisermos indicar falha total
        }

        print server, file, avg_rps, s_min_rps, s_max_rps, avg_tpr, s_min_tpr, s_max_tpr, s_falhas
    }
}
' "${SUMMARY_CSV_FILE}" > "${STATS_CSV_FILE}"

echo "Estatísticas agregadas salvas em: ${STATS_CSV_FILE}"
echo "Resultados detalhados em: ${SUMMARY_CSV_FILE}"
echo "Dados brutos do 'ab' em: ${RAW_DATA_DIR}"
echo ">>> Processamento concluído. <<<"

exit 0
