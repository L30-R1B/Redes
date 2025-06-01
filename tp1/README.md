# Sistema de Transferência de Arquivos - Protocolos TCP e UDP

## Visão Geral

Este projeto implementa um sistema de transferência de arquivos utilizando os protocolos TCP e UDP, permitindo que clientes solicitem arquivos de um servidor com verificação de integridade. O sistema inclui:

- **Servidor/Cliente TCP**: Transferência confiável com comunicação orientada a conexão
- **Servidor/Cliente UDP**: Transferência eficiente com comunicação baseada em pacotes e tratamento de perdas
- **Gerador de Arquivos**: Utilitário para criar arquivos de teste de diversos tamanhos

## Funcionalidades

### Comuns a TCP e UDP

- Listagem de arquivos (comando `list`)
- Transferência de arquivos (comando `get <arquivo>`)
- Verificação de integridade com checksum MD5
- Registro de log com estatísticas detalhadas
- Criação automática de diretórios para caminhos aninhados
- Tratamento de timeouts

### Exclusivas do TCP

- Entrega confiável e em ordem
- Comunicação baseada em fluxo
- Gerenciamento de conexões
- Atendimento paralelo a clientes (usando fork)

### Exclusivas do UDP

- Comunicação baseada em pacotes com números de sequência
- Detecção de pacotes duplicados
- Estatísticas de perda de pacotes
- Mecanismo de timeout e retentativas

## Componentes

### 1. Implementação TCP

#### Servidor (`server_tcp.c`)
- Escuta na porta 8080 (configurável)
- Atende múltiplos clientes usando fork()
- Valida caminhos de arquivos por segurança
- Registra detalhes da transferência em `server_tcp.log`

#### Cliente (`client_tcp.c`)
- Conecta ao IP e porta do servidor especificados
- Implementa timeout para operações de envio/recebimento
- Verifica integridade dos arquivos com checksum MD5
- Registra detalhes da transferência em `client_tcp.log`

### 2. Implementação UDP

#### Servidor (`server_udp.c`)
- Escuta na porta 8080 (configurável)
- Comunicação baseada em pacotes com números de sequência
- Contabiliza número de pacotes enviados
- Registra detalhes da transferência em `server_udp.log`

#### Cliente (`client_udp.c`)
- Conecta ao IP e porta do servidor especificados
- Trata perda e reordenação de pacotes
- Calcula porcentagem de perda de pacotes
- Registra detalhes da transferência em `client_udp.log`

### 3. Gerador de Arquivos (`file_generator.c`)

Utilitário para criar arquivos de teste com tamanhos variados e conteúdo aleatório:

```bash
./file_generator <MAX_N> <TAMANHO_BLOCO>
```

- Cria arquivos nomeados `file-1.txt` até `file-N.txt`
- Cada arquivo contém `N * TAMANHO_BLOCO` bytes de caracteres imprimíveis aleatórios
- Útil para testar confiabilidade e desempenho da transferência

## Como Usar

### Configuração do Servidor

1. **Servidor TCP**:
   ```bash
   ./server_tcp [diretorio_raiz]
   ```

2. **Servidor UDP**:
   ```bash
   ./server_udp [diretorio_raiz]
   ```

### Comandos do Cliente

1. **Cliente TCP**:
   ```bash
   ./client_tcp <ip_servidor> <porta>
   ```

2. **Cliente UDP**:
   ```bash
   ./client_udp <ip_servidor> <porta>
   ```

Comandos disponíveis no cliente:
- `list` - Lista arquivos disponíveis
- `get <arquivo>` - Baixa um arquivo
- `exit` - Sai do cliente

## Detalhes Técnicos

### Processo de Transferência

1. Cliente envia requisição (`get <arquivo>`)
2. Servidor responde com metadados (tamanho + checksum MD5)
3. Transferência dos dados do arquivo
4. Cliente verifica o arquivo recebido usando o checksum
5. Estatísticas da transferência registradas em ambos os lados

### Estrutura do Pacote UDP

```c
typedef struct {
    int packet_number;    // Número de sequência
    char is_last;         // Flag para último pacote
    char data[BUFFER_SIZE]; // Dados
} Packet;
```

### Registro de Logs

Tanto clientes quanto servidores mantêm logs detalhados incluindo:
- Data e hora
- Duração da transferência
- Velocidade de transferência
- Tamanho do arquivo
- Checksums MD5
- Estatísticas de pacotes (apenas UDP)
- Status (sucesso/falha)

## Considerações de Desempenho

- **TCP**: Melhor para transferências confiáveis, com controle de congestionamento automático
- **UDP**: Menor overhead, mas requer tratamento manual de perda e ordenação de pacotes
- **Tamanho do Buffer**: Padrão de 4096 bytes, pode ser ajustado para performance
- **Timeouts**: Configuráveis (5s para TCP, 1s por pacote para UDP)

## Segurança

- Validação de caminhos para prevenir directory traversal
- Verificação de integridade com checksum MD5
- Timeouts de conexão para prevenir travamentos

## Testes

O `file_generator.c` incluído pode criar arquivos de teste de diversos tamanhos para avaliar:
- Confiabilidade da transferência
- Desempenho com diferentes tamanhos de arquivo
- Comportamento dos protocolos com perda de pacotes (UDP)

Exemplo de geração de arquivos de teste:
```bash
./file_generator 10 1024  # Cria 10 arquivos de 1KB a 10KB
```

## Conclusão

Este sistema fornece uma comparação abrangente da transferência de arquivos usando os protocolos TCP e UDP, demonstrando seus pontos fortes e compensações. Os mecanismos detalhados de registro e verificação garantem operação confiável enquanto fornecem métricas valiosas de desempenho.
