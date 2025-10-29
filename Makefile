CC = gcc
CFLAGS = -Wall -O2

all: cliente servidor

cliente: client/cliente_http.c
	$(CC) $(CFLAGS) -o cliente client/cliente_http.c

servidor: server/servidor_http.c
	$(CC) $(CFLAGS) -o servidor server/servidor_http.c

clean:
	rm -f cliente servidor
