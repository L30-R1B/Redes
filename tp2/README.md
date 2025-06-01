+++ b/home/leonardo/Redes/tp2/README.md
@@ -0,0 +1,167 @@
+# Projeto de Comparativo de Desempenho de Servidores Web em C
+
+Este projeto implementa e testa diferentes modelos de concorrência para servidores web simples escritos em C. O objetivo principal é comparar o desempenho desses modelos sob diversas cargas de requisição e níveis de concorrência.
+
+## Funcionalidades
+
+*   **Múltiplos Modelos de Servidor:**
+    *   `servidor_iterativo`: Processa uma requisição por vez.
+    *   `servidor_fork`: Cria um novo processo para cada requisição (cliente).
+    *   `servidor_thread_per_cliente`: Cria uma nova thread para cada requisição (cliente).
+    *   `servidor_thread_pool`: Utiliza um pool de threads pré-alocadas para lidar com as requisições.
+    *   `servidor_select`: Utiliza a chamada de sistema `select()` para E/S não bloqueante, gerenciando múltiplos clientes em uma única thread.
+*   **Serviço de Arquivos Estáticos:** Os servidores são capazes de servir arquivos estáticos (ex: imagens PNG) localizados no diretório raiz do projeto.
+*   **Suite de Testes Automatizada:** Um script (`test/test_suite.sh`) automatiza a execução dos testes de desempenho usando o Apache Benchmark (`ab`).
+*   **Coleta de Métricas:** O script coleta métricas chave como:
+    *   Requisições Por Segundo (RPS)
+    *   Tempo Por Requisição (TPR) em milissegundos
+    *   Número de Falhas
+*   **Relatórios Detalhados:** Os resultados são salvos em arquivos CSV para fácil análise:
+    *   `summary_all_tests.csv`: Resultados detalhados de cada combinação de servidor, arquivo e concorrência.
+    *   `aggregated_stats.csv`: Estatísticas agregadas (média, mínimo, máximo) por servidor e arquivo.
+*   **Dados Brutos:** Os dados brutos de saída do `ab` (formato TSV para `-g`) são salvos para análises mais profundas.
+
+## Estrutura do Projeto
+
+```
+tp2/
+├── common/             # Módulos de código comuns (socket utils, logger, mime_types, http_handler)
+│   ├── http_handler.c
+│   ├── http_handler.h
+│   ├── logger.c
+│   ├── logger.h
+│   ├── mime_types.c
+│   ├── mime_types.h
+│   ├── socket_utils.c
+│   └── socket_utils.h
+├── servers/            # Código fonte dos diferentes servidores
+│   ├── servidor_fork.c
+│   ├── servidor_iterativo.c
+│   ├── servidor_select.c
+│   ├── servidor_thread_per_cliente.c
+│   └── servidor_thread_pool.c
+├── test/               # Scripts e utilitários de teste
+│   └── test_suite.sh
+├── test_results/       # Diretório gerado para armazenar os resultados dos testes (ignorado pelo git)
+├── Makefile.mk         # Makefile para compilação do projeto
+└── README.md           # Este arquivo
+```
+Os arquivos de imagem para teste (ex: `image.png`, `image2.png`) devem estar presentes no diretório raiz do projeto (`tp2/`) para que os servidores possam encontrá-los.
+
+## Pré-requisitos
+
+Para compilar e executar os testes, você precisará ter os seguintes softwares instalados:
+
+*   **GCC (GNU Compiler Collection):** Para compilar o código C.
+*   **Make:** Para automatizar o processo de compilação usando o `Makefile.mk`.
+*   **Apache Benchmark (`ab`):** Ferramenta para testes de carga em servidores HTTP.
+    *   Instalação (Debian/Ubuntu): `sudo apt-get update && sudo apt-get install apache2-utils`
+*   **lsof:** Utilitário para listar arquivos abertos (usado pelo script de teste para verificar e liberar portas).
+    *   Instalação (Debian/Ubuntu): `sudo apt-get install lsof`
+
+## Como Compilar
+
+Navegue até o diretório raiz do projeto (`/home/leonardo/Redes/tp2/` ou o caminho correspondente no seu sistema) e execute o comando `make` especificando o arquivo `Makefile.mk`:
+
+```bash
+cd /caminho/para/tp2
+make -f Makefile.mk
+```
+
+Isso compilará todos os módulos comuns e os executáveis dos servidores, que serão colocados no diretório raiz do projeto (ex: `servidor_iterativo`, `servidor_fork`, etc.).
+
+Para limpar os arquivos compilados e os resultados dos testes:
+
+```bash
+make -f Makefile.mk clean
+```
+
+## Como Executar os Testes
+
+Após a compilação bem-sucedida dos servidores, você pode executar a suíte de testes.
+
+1.  Certifique-se de que os arquivos de imagem que serão requisitados (configurados em `ARQUIVOS_TESTE` dentro do script `test_suite.sh`, por exemplo, `/image.png`) estejam presentes no diretório raiz do projeto (`tp2/`).
+2.  Navegue até o diretório `test/` e execute o script `test_suite.sh`:
+
+    ```bash
+    cd test/
+    ./test_suite.sh
+    ```
+
+O script realizará os seguintes passos:
+*   Verificará as dependências (`ab`, `lsof`).
+*   Criará um diretório com timestamp dentro de `test_results/` para armazenar os dados desta execução.
+*   Para cada servidor configurado:
+    *   Para cada arquivo de teste e nível de concorrência:
+        *   Tentará garantir que a porta de teste (padrão `8088`) esteja livre, encerrando processos que a estejam utilizando (pode requerer permissões elevadas se os processos forem de outro usuário).
+        *   Iniciará o servidor na porta especificada.
+        *   Executará o `ab` com os parâmetros definidos para número de requisições e concorrência.
+        *   Coletará as métricas de desempenho.
+        *   Salvará os resultados no arquivo `summary_all_tests.csv` e os dados brutos do `ab` em `test_results/<timestamp>/raw_data/`.
+        *   Parará o servidor.
+*   Ao final, gerará um arquivo `aggregated_stats.csv` com estatísticas consolidadas.
+
+**Observação sobre a liberação da porta:** O script tenta liberar a porta `8088` automaticamente. Se ele não conseguir (por exemplo, se um processo de outro usuário ou do sistema estiver usando a porta), uma mensagem de erro será exibida, e pode ser necessário liberar a porta manualmente com `sudo kill <PID>` ou `sudo kill -9 <PID>`.
+
+## Interpretação dos Resultados
+
+Os resultados dos testes são salvos no diretório `test_results/` dentro de uma subpasta nomeada com a data e hora da execução (ex: `test_results/2023-10-27_10-30-00/`).
+
+*   **`summary_all_tests.csv`**: Contém uma linha para cada combinação de servidor, arquivo requisitado e nível de concorrência testado. As colunas incluem:
+    *   `Servidor`: Nome do executável do servidor.
+    *   `Arquivo`: Caminho do arquivo requisitado.
+    *   `Concorrencia`: Nível de concorrência usado no teste `ab`.
+    *   `ReqsPorSegundo`: Requisições por segundo (RPS).
+    *   `TempoPorReq_ms`: Tempo médio por requisição em milissegundos.
+    *   `Falhas`: Número de requisições falhas.
+    *   Valores como `ERRO_INICIO`, `ERRO_AB`, `ERRO_PORTA` indicam problemas durante o teste específico.
+
+*   **`aggregated_stats.csv`**: Fornece estatísticas agregadas (média, mínimo, máximo de RPS e TPR, e total de falhas) para cada par `Servidor,Arquivo` ao longo de todos os níveis de concorrência testados.
+
+*   **`raw_data/`**: Este subdiretório contém:
+    *   `raw_data/<nome_servidor>/ab_data_<nome_servidor>_c<concorrencia>_f<arquivo>.tsv`: Arquivos TSV gerados pela opção `-g` do `ab`, contendo dados detalhados de tempo para cada requisição. Útil para análises mais profundas ou para gerar gráficos de distribuição de tempo de resposta.
+
+## Configurações dos Testes
+
+As principais configurações da suíte de testes podem ser ajustadas no início do script `test/test_suite.sh`:
+
+*   `PORTA_TESTE`: Porta em que os servidores serão iniciados (padrão: `8088`).
+*   `ARQUIVOS_TESTE`: Array com os caminhos dos arquivos a serem requisitados (ex: `"/image.png"`).
+*   `CONCORRENCIAS`: Array com os diferentes níveis de concorrência a serem testados pelo `ab` (ex: `1 5 10 20 50`).
+*   `NUM_REQUISICOES`: Número total de requisições que o `ab` fará em cada teste (padrão: `1000`).
+
+## Trabalhos Futuros / Possíveis Melhorias
+
+*   **Novos Modelos de Servidor:** Implementar e testar servidores baseados em `epoll` (Linux) ou `kqueue` (BSD/macOS) para comparação com `select`.
+*   **Variação de Carga:** Testar com diferentes tamanhos e tipos de arquivos.
+*   **Geração Automática de Gráficos:** Adicionar funcionalidade para gerar gráficos comparativos de desempenho (RPS vs Concorrência, TPR vs Concorrência) a partir dos dados CSV ou TSV.
+*   **Tratamento de Erros:** Aprimorar o tratamento de erros e o logging nos servidores.
+*   **Análise de Recursos:** Integrar ferramentas para monitorar o uso de CPU e memória pelos servidores durante os testes.
+
+---
+
+Este README deve fornecer uma boa base para entender, compilar e utilizar o projeto.
+```

Espero que este README.md seja útil para o seu projeto! Ele detalha a estrutura, como compilar, executar os testes e interpretar os resultados, além de incluir as dependências e configurações importantes.
