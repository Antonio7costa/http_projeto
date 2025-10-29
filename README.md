# Servidor e Cliente HTTP em C

Este é um projeto acadêmico que implementa um cliente e um servidor HTTP simples em C, focado em entender os fundamentos do protocolo HTTP e da programação de sockets.

O projeto consiste em duas partes:
1.  **Servidor:** Um servidor web que serve arquivos de um diretório local.
2.  **Cliente:** Um "navegador" de linha de comando (similar ao `wget`) que baixa arquivos de um URL.

## Funcionalidades Implementadas

### Servidor (`servidor`)
* Inicia um servidor local na porta `5050`.
* Serve arquivos de um diretório-raiz especificado na linha de comando.
* Serve automaticamente o arquivo `index.html` caso a requisição seja para o diretório raiz (`/`).
* Retorna uma listagem em HTML dos arquivos do diretório caso o `index.html` não exista ou um arquivo não seja encontrado.
* Utiliza a opção de socket `SO_REUSEADDR` para permitir que o servidor seja reiniciado rapidamente sem erros de "Address already in use".

### Cliente (`cliente`)
* Faz o download de um arquivo de um URL e o salva no diretório atual.
* Extrai o nome do arquivo do URL para usá-lo como nome do arquivo salvo.
* Implementa um **timeout de 5 segundos** para conexões e recebimento de dados, evitando que o terminal "trave" indefinidamente.
* (Correção) Envia a requisição com a barra inicial (ex: `GET /pagina1.txt ...`) para compatibilidade com o servidor.

## Requisitos para Compilação
* Um compilador C (ex: `gcc`)
* A ferramenta `make`

## Como Compilar

O projeto inclui um `Makefile` para facilitar a compilação.

1.  Para compilar os dois executáveis (`cliente` e `servidor`), execute:
    ```bash
    make
    ```
   

2.  Para limpar os arquivos compilados, execute:
    ```bash
    make clean
    ```
   

## Como Testar (Cliente vs. Servidor Local)

Para o teste completo, você precisará de dois terminais abertos no diretório do projeto.

**Nota:** O código do cliente está atualmente com a porta "fixa" em `5050` para este teste local.

### Terminal 1: Iniciar o Servidor

1.  Primeiro, crie um diretório de teste e um arquivo de exemplo:
    ```bash
    mkdir meu_site_teste
    echo 'Este e um arquivo de teste.' > meu_site_teste/pagina1.txt
    ```

2.  Inicie o servidor, apontando para o diretório que você acabou de criar:
    ```bash
    ./servidor meu_site_teste
    ```

3.  O servidor deverá exibir a mensagem e permanecerá rodando:
    ```
    Servidor rodando em http://localhost:5050/
    ```
   

### Terminal 2: Executar o Cliente

1.  Com o servidor rodando no Terminal 1, execute o cliente no Terminal 2, pedindo o arquivo que você criou:
    ```bash
    ./cliente http://localhost/pagina1.txt
    ```

2.  O cliente irá se conectar ao servidor na porta 5050, baixar o arquivo e salvá-lo.
    **Saída Esperada:**
    ```
    Arquivo salvo em: pagina1.txt
    ```
   

3.  Você pode verificar o conteúdo do arquivo baixado (que deve ser uma cópia) com:
    ```bash
    cat pagina1.txt
    ```
    **Saída Esperada:**
    ```
    Este e um arquivo de texto.
    ```

## Limitações e Problemas Conhecidos

* **Parse de URL (Cliente):** O cliente não faz o "parse" da porta (ex: `:5050`) a partir do URL. A porta de conexão está "fixa" no código (`hard-coded`) em `cliente_http.c` (linha `server_addr.sin_port = htons(5050);`).
* **Teste Externo (Porta 80):** Para testar o cliente com um site externo (ex: `http://example.com/index.html`), é necessário alterar manualmente a porta no `cliente_http.c` para `htons(80)`, salvar e recompilar com `make`.
* **HTTP vs HTTPS:** O cliente só fala HTTP. Ele não funcionará com sites `https://`.
* **Redirecionamentos:** O cliente não lida com redirecionamentos HTTP (Status 301/302).
* **Erros do Servidor:** O servidor não retorna `404 Not Found`. Em vez disso, ele retorna a listagem do diretório raiz se um arquivo não for encontrado.

## Licença

Este projeto está licenciado sob a Licença MIT.