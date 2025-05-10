# Script de Testes Comparativos: TCP vs UDP

## `test.sh`

### Descrição
Este script automatizado realiza testes comparativos entre os protocolos TCP e UDP para transferência de arquivos, gerando relatórios detalhados de desempenho.

### Funcionalidades Principais
- Geração automática de arquivos de teste
- Execução de baterias de testes organizadas
- Coleta de métricas de desempenho
- Geração de relatórios comparativos
- Organização estruturada dos resultados

### Como Executar
```bash
./test.sh <MAX_N> <CHUNK_SIZE> <NUM_TESTS>
```

### Parâmetros
| Parâmetro    | Descrição                                                                 |
|--------------|---------------------------------------------------------------------------|
| `MAX_N`      | Número máximo de arquivos a serem gerados (de 1 até MAX_N)                |
| `CHUNK_SIZE` | Tamanho base para geração dos arquivos (em bytes)                         |
| `NUM_TESTS`  | Número de repetições para cada teste (para aumentar a confiabilidade)     |

### Estrutura de Diretórios Criada
```
test_results_<DATA_HORA>/
├── client_logs/
│   ├── tcp/
│   │   └── test_<N>/ (logs individuais)
│   └── udp/
│       └── test_<N>/ (logs individuais)
├── server_logs/
├── tcp_results.csv (dados brutos)
├── udp_results.csv (dados brutos)
├── tcp_report.txt (análise)
├── udp_report.txt (análise)
└── comparative_report.txt (comparativo)
```

### Métricas Coletadas
Para cada transferência:
- Tempo de transferência (segundos)
- Velocidade (KB/s)
- Status (OK/FALHA)
- Pacotes enviados/recebidos (UDP)
- Taxa de perda de pacotes (UDP)

### Relatórios Gerados
1. **Relatórios Individuais** por protocolo com:
   - Tempo médio de transferência
   - Velocidade média
   - Taxa de sucesso
   - Perda média de pacotes (UDP)

2. **Relatório Comparativo** com tabela comparando:
   - Tempos médios (TCP vs UDP)
   - Velocidades médias (TCP vs UDP)
   - Por tamanho de arquivo

### Fluxo de Execução
1. Prepara ambiente (diretórios, limpeza)
2. Gera arquivos de teste
3. Inicia servidores TCP/UDP
4. Executa bateria de testes:
   - Para cada protocolo (TCP, UDP)
   - Para cada número de teste (1..NUM_TESTS)
   - Para cada arquivo (file-1.txt..file-MAX_N.txt)
5. Processa resultados
6. Gera relatórios
7. Finaliza servidores e organiza logs

### Exemplo de Uso
```bash
# Testar com 5 arquivos (1KB a 5KB), 3 repetições
./test.sh 5 1024 3
```

### Saída Esperada
- Arquivos CSV com dados brutos
- Relatórios textuais analisados
- Logs detalhados de cada execução
- Resumo comparativo final

### Personalização
O script pode ser adaptado para:
- Alterar IP/porta do servidor
- Modificar tamanhos de buffer
- Ajustar tempos de espera
- Incluir novos tipos de teste

Observação: Requer os binários do servidor/cliente e do gerador de arquivos compilados no diretório `bin/`.
