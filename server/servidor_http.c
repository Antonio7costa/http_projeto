#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUFFER_SIZE 4096

void send_file(int client, const char *path) {
    /* Tenta abrir o arquivo pedido em modo "leitura binária" (rb). */
    FILE *file = fopen(path, "rb");

    /* Se não conseguir abrir (arquivo não existe), envia um erro 404. */
    if (!file) {
        char *msg = "HTTP/1.0 404 Not Found\r\n\r\nArquivo não encontrado.";
        send(client, msg, strlen(msg), 0);
        return;
    }

    char header[128];
    snprintf(header, sizeof(header), "HTTP/1.0 200 OK\r\n\r\n");
    send(client, header, strlen(header), 0);

    /* Lê o arquivo em "pedaços" (chunks) de 4KB... */
    char buffer[BUFFER_SIZE];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client, buffer, bytes, 0);
    }

    fclose(file);
}


void list_directory(int client, const char *dirpath) {
    DIR *dir = opendir(dirpath);
    if (!dir) {
        char *msg = "HTTP/1.0 404 Not Found\r\n\r\nDiretório não encontrado.";
        send(client, msg, strlen(msg), 0);
        return;
    }

    char response[BUFFER_SIZE] = "HTTP/1.0 200 OK\r\n\r\n<html><body><ul>";

    /* Lê cada item (arquivo/pasta) dentro do diretório. */
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        strcat(response, "<li>");
        strcat(response, entry->d_name);
        strcat(response, "</li>");
    }
    strcat(response, "</ul></body></html>");

    /* Envia a página HTML pronta para o cliente. */
    send(client, response, strlen(response), 0);
    closedir(dir);
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <diretório>\n", argv[0]);
        return 1;
    }

    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    /* 1. Cria o 'conector' (socket) principal do servidor. */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { 
        perror("socket");
        exit(EXIT_FAILURE);
    }


    /* Esta opção permite ao servidor reiniciar rápido sem o erro "Address already in use". */
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }


    /* 2. Define as configurações: porta 5050 e escutar em qualquer IP local. */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5050);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    /* 3. "Amarra" (bind) o conector à porta 5050. */
    /* Adicionada verificação de erro para o bind() */
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    
    /* 4. Coloca o servidor em modo de "escuta" (pronto para aceitar clientes). */
    if (listen(server_fd, 5) < 0) { 
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Servidor rodando em http://localhost:5050/\n");

    /* --- LOOP PRINCIPAL DO SERVIDOR --- */
    while (1) {
        /* 5. PAUSA e espera por uma nova conexão */
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) { 
            perror("accept");
            continue; 
        }
        
        /* 6. Recebe a mensagem (requisição) do cliente */
        int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            close(client_fd);
            continue;
        }

        /* 7. "Lê" a requisição para descobrir o 'caminho' pedido. */
        buffer[bytes] = '\0';
        char method[8], path[512];
        sscanf(buffer, "%s %s", method, path);


        /* 8. Monta o caminho completo do arquivo no disco. (SEM o caso especial /index.html) */
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s%s", argv[1], path);

        /* 9. Verifica o que existe neste caminho. */
        struct stat st;
        if (stat(filepath, &st) != 0) {
            char *msg = "HTTP/1.0 404 Not Found\r\n\r\nO arquivo ou diretorio nao foi encontrado.";
            send(client_fd, msg, strlen(msg), 0);

        } else if (S_ISREG(st.st_mode)) {
            send_file(client_fd, filepath);

        } else if (S_ISDIR(st.st_mode)) {
            
            // Tenta encontrar um 'index.html' DENTRO deste diretório
            char index_path[600];
            snprintf(index_path, sizeof(index_path), "%s/index.html", filepath);

            struct stat index_st;
            if (stat(index_path, &index_st) == 0 && S_ISREG(index_st.st_mode)) {
                send_file(client_fd, index_path);
            } else {
                list_directory(client_fd, filepath);
            }
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}