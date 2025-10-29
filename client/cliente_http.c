#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#define BUFFER_SIZE 4096

void download_file(const char *host, const char *path, const char *filename) {
    int sock;
    struct hostent *server;
    struct sockaddr_in server_addr;
    char request[1024], buffer[BUFFER_SIZE];
    FILE *output;

    /* Converte o nome (ex: "localhost") em um endereço IP. */
    server = gethostbyname(host);
    if (!server) {
        fprintf(stderr, "Erro: host não encontrado.\n");
        exit(1);
    }

    /* Pede ao sistema operacional um "conector" de rede. */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Erro ao criar socket");
        exit(1);
    }

    /* Define um limite de 5 segundos para a conexão não "travar". */
    struct timeval timeout;
    timeout.tv_sec = 5;  // 5 segundos
    timeout.tv_usec = 0; // 0 microssegundos

    // Configura o timeout para recebimento (afeta connect() e recv())
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        perror("Erro ao configurar timeout de recebimento");
    }

    // Configura o timeout para envio (afeta send())
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        perror("Erro ao configurar timeout de envio");
    }

    /* Prepara a "ficha" com o IP e a Porta para onde vamos conectar. */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5050); // Porta 5050
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);

    /* Tenta se conectar ao servidor. */
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro na conexão");
        close(sock);
        exit(1);
    }

    /* Monta e envia a mensagem de texto (requisição HTTP) pedindo o arquivo. */
    snprintf(request, sizeof(request),
             "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host);
    send(sock, request, strlen(request), 0);

    /* Cria o arquivo local onde o conteúdo será salvo. */
    output = fopen(filename, "wb");
    if (!output) {
        perror("Erro ao criar arquivo");
        close(sock);
        exit(1);
    }

    /* Inicia o loop de download. */
    int header_passed = 0;
    while (1) {
        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;

        /* Lógica para pular o cabeçalho (ex: "HTTP/1.1 200 OK"). */
        if (!header_passed) {
            char *body = strstr(buffer, "\r\n\r\n");
            if (body) {
                header_passed = 1;
                body += 4;
                fwrite(body, 1, bytes - (body - buffer), output);
            }
        } else {
            /* Depois que o cabeçalho passou, salva tudo que vier. */
            fwrite(buffer, 1, bytes, output);
        }
    }

    /* Limpeza Final. */
    fclose(output);
    close(sock);
    printf("Arquivo salvo em: %s\n", filename);
}

/**
 * Ponto de entrada do programa.
 */
int main(int argc, char *argv[]) {
    // Garante que o usuário passou um URL.
    if (argc != 2) {
        printf("Uso: %s http://host/caminho/arquivo\n", argv[0]);
        return 1;
    }

    char host[256], path[512], *url = argv[1];

    /* "Lê" o URL e o quebra em "host" e "caminho". */
    if (sscanf(url, "http://%255[^/]/%511[^\n]", host, path) != 2) {
        fprintf(stderr, "URL inválida. Use o formato http://host/arquivo\n");
        return 1;
    }

    /* Pega o "caminho" (ex: "pasta/foto.jpg") e extrai só o nome ("foto.jpg"). */
    char *filename = strrchr(path, '/');
    
    if (filename) {
        // Se achou uma barra (ex: /pasta/pagina.html), avança para depois dela
        filename = filename + 1;
    } else {
        // Se não achou barra (ex: "pagina.html"), o caminho todo é o nome do arquivo
        filename = path;
    }

    /* --- INÍCIO DA ALTERAÇÃO --- */
    
    // Variáveis para guardar o caminho final e o nome do arquivo final
    char final_path[512];
    char final_filename[256];

    // Se o nome do arquivo extraído estiver VAZIO (ex: URL era "http://localhost/" ou "http://localhost/pasta/")
    if (strlen(filename) == 0) {
        
        // O nome do arquivo para salvar será "index.html"
        strcpy(final_filename, "index.html");
        
        // O caminho para requisitar ao servidor será "path" + "index.html"
        // (ex: "" + "index.html" ou "pasta/" + "index.html")
        strcpy(final_path, path);
        strcat(final_path, "index.html");
        
    } else {
         // Caso normal: o nome do arquivo não estava vazio
        strcpy(final_path, path);
        strcpy(final_filename, filename);
    }
    
    // Chama a função com os nomes corrigidos
    download_file(host, final_path, final_filename);
    
    /* --- FIM DA ALTERAÇÃO --- */

    return 0;
}