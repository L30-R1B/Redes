# Protocolo de Transferência Confiável Stop-and-Wait sobre UDP

## Descrição

Este projeto é uma implementação de um protocolo de transferência de arquivos confiável que opera sobre a camada de transporte UDP. [cite\_start]Ele segue o modelo **Stop-and-Wait** para garantir a entrega correta e completa de um arquivo entre um cliente e um servidor, mesmo em um ambiente com simulação de perdas de pacotes[cite: 6, 7, 10].

[cite\_start]Esta é a implementação do **Trabalho Prático 3** da disciplina de Redes de Computadores da Universidade Federal de São João del-Rei (UFSJ)[cite: 3, 4].

## Funcionalidades Implementadas

[cite\_start]O sistema foi desenvolvido para atender a todos os requisitos obrigatórios, incluindo[cite: 12]:

  * **Protocolo Stop-and-Wait:** O cliente envia um pacote de dados e aguarda a confirmação (ACK) do servidor antes de enviar o próximo. Números de sequência (0 e 1) são usados para identificar pacotes e duplicatas.
  * [cite\_start]**Handshake de Conexão:** A comunicação é iniciada com um par de mensagens `START`/`START_ACK` e finalizada com `END`/`END_ACK` para garantir que o servidor esteja pronto e que a transmissão foi concluída[cite: 13, 14].
  * **Checksum de 8 bits:** Cada pacote inclui um checksum simples (soma de todos os bytes) para verificar a integridade dos dados. [cite\_start]Pacotes com checksum inválido são descartados pelo servidor[cite: 15, 16].
  * **Timeout e Retransmissão:** O cliente implementa um mecanismo de timeout. Se um ACK não for recebido dentro do tempo esperado, o pacote é retransmitido. Um número máximo de tentativas é definido para evitar loops infinitos.
  * [cite\_start]**Simulação de Perda:** O servidor pode simular a perda de pacotes com uma taxa configurável via linha de comando (0-100%)[cite: 20].
  * [cite\_start]**Logs e Modo Verbose:** Ambos, cliente e servidor, podem exibir logs detalhados de suas operações (pacotes enviados, recebidos, timeouts, etc.) usando a flag `-v` na linha de comando[cite: 17, 18].
  * [cite\_start]**Estatísticas de Desempenho:** Ao final da transmissão, o cliente e o servidor exibem estatísticas detalhadas, como tempo total, taxa de transferência, total de retransmissões e pacotes com erro[cite: 21].

## Estrutura do Projeto

```
.
├── client.c              # Código fonte do cliente
├── server.c              # Código fonte do servidor
├── protocol.h            # Header com definições do protocolo (pacote, tipos, etc.)
├── script.sh             # Script de teste automatizado
├── Makefile              # Arquivo para compilação do projeto
└── README.md             # Este arquivo
```

## Como Compilar e Executar

**Pré-requisitos:**

  * Compilador C (GCC)
  * `make`

**1. Compilação**

Para compilar o cliente e o servidor, execute o seguinte comando no terminal:

```bash
make
```

Isso gerará dois executáveis: `client` e `server`.

**2. Execução**

Primeiro, inicie o servidor em um terminal:

```bash
./server -p <porta> -l <taxa_de_perda> [-v]
```

  * `-p <porta>`: Porta em que o servidor irá escutar.
  * `-l <taxa_de_perda>`: Taxa de perda de pacotes em porcentagem (0-100).
  * `-v`: (Opcional) Ativa o modo verbose para exibir logs detalhados.

**Exemplo (Servidor):**

```bash
./server -p 8080 -l 10 -v
```

Em seguida, execute o cliente em outro terminal para enviar o arquivo:

```bash
./client -h <host> -p <porta> -f <arquivo> [-v]
```

  * `-h <host>`: Endereço IP ou hostname do servidor.
  * `-p <porta>`: Porta em que o servidor está escutando.
  * `-f <arquivo>`: Caminho do arquivo a ser enviado.
  * `-v`: (Opcional) Ativa o modo verbose.

**Exemplo (Cliente):**

```bash
./client -h 127.0.0.1 -p 8080 -f meu_arquivo.txt -v
```

## Automação de Testes

O projeto inclui um script (`script.sh`) que automatiza a execução de uma bateria de testes, variando a taxa de perda de 0% a 100%.

Para executar o script:

```bash
bash script.sh
```

O script irá:

1.  Compilar o projeto.
2.  Criar um arquivo de teste (`original_file.txt`).
3.  Executar o cliente e o servidor para cada taxa de perda.
4.  Verificar se o arquivo recebido é idêntico ao original.
5.  Salvar todos os logs e um arquivo `results.csv` com as estatísticas em um novo diretório dentro de `test_results/`.

## Detalhes do Protocolo

### Estrutura do Pacote

A estrutura do pacote está definida em `protocol.h` da seguinte forma:

  * `uint32_t seq_num`: Número de sequência do pacote (para pacotes `DATA`).
  * `uint32_t ack_num`: Número de sequência que está sendo confirmado (para pacotes `ACK`).
  * `uint8_t type`: Tipo do pacote (veja abaixo).
  * `uint8_t checksum`: Checksum de 8 bits para verificação de integridade.
  * `uint16_t payload_size`: Tamanho dos dados no payload.
  * `char payload[1024]`: Carga útil de dados.

### Tipos de Pacote

  * `TYPE_START (1)`: Inicia a conexão (cliente -\> servidor).
  * `TYPE_DATA (2)`: Contém um fragmento do arquivo (cliente -\> servidor).
  * `TYPE_ACK (3)`: Confirma o recebimento de um pacote de dados (servidor -\> cliente).
  * `TYPE_END (4)`: Finaliza a conexão (cliente -\> servidor).
  * `TYPE_END_ACK (5)`: Confirma o fim da conexão (servidor -\> cliente).
  * `TYPE_START_ACK (6)`: Confirma o início da conexão (servidor -\> cliente).
