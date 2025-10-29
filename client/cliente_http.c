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

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        perror("Erro ao configurar timeout de recebimento");
    }
    
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        perror("Erro ao configurar timeout de envio");
    }

    /* Prepara a "ficha" com o IP e a Porta para onde vamos conectar. */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5050);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro na conexão");
        close(sock);
        exit(1);
    }

    /* Monta e envia a mensagem de texto (requisição HTTP) pedindo o arquivo. */
    snprintf(request, sizeof(request),
             "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host);
    send(sock, request, strlen(request), 0);


    /* Recebe o início da resposta (o cabeçalho) */
    int bytes = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) {
        perror("Erro ao receber dados do servidor");
        close(sock);
        exit(1);
    }
    buffer[bytes] = '\0';

    if (strstr(buffer, "200 OK") == NULL) {
        
        // Pega a primeira linha da resposta para mostrar o erro
        char status_line[128];
        strncpy(status_line, buffer, sizeof(status_line) - 1);
        status_line[sizeof(status_line) - 1] = '\0';
        
        // Remove a quebra de linha (\r\n) do final
        char *end_of_line = strstr(status_line, "\r\n");
        if (end_of_line) {
            *end_of_line = '\0';
        }

        // Imprime o erro 404 no terminal e NÃO salva o arquivo.
        fprintf(stderr, "Erro ao baixar: O servidor retornou o status: %s\n", status_line);
        close(sock);
        exit(1); // Encerra o programa
    }

    /* Se for "200 OK", encontramos o corpo e salvamos o arquivo. */

    /* Procura o fim do cabeçalho (\r\n\r\n) */
    char *body = strstr(buffer, "\r\n\r\n");
    if (!body) {
        fprintf(stderr, "Erro: Resposta do servidor mal formatada (sem corpo).\n");
        close(sock);
        exit(1);
    }
    body += 4;

    /* SÓ AGORA (depois de confirmar o 200 OK) criamos o arquivo. */
    output = fopen(filename, "wb");
    if (!output) {
        perror("Erro ao criar arquivo");
        close(sock);
        exit(1);
    }

    // Escreve o primeiro "pedaço" do arquivo que já recebemos
    size_t body_bytes_in_buffer = bytes - (body - buffer);
    fwrite(body, 1, body_bytes_in_buffer, output);

    // Continua o loop para baixar o resto do arquivo
    while ((bytes = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes, output);
    }


    fclose(output);
    close(sock);
    printf("Arquivo salvo em: %s\n", filename);
}


int main(int argc, char *argv[]) {
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
        filename = filename + 1;
    } else {
        filename = path;
    }


    char final_path[512];
    char final_filename[256];

    if (strlen(filename) == 0) {
        strcpy(final_filename, "index.html");
        strcpy(final_path, path);
        strcat(final_path, "index.html");
        
    } else {
        strcpy(final_path, path);
        strcpy(final_filename, filename);
    }

    download_file(host, final_path, final_filename);

    return 0;
}