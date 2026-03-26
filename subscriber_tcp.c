#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5000

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char *message = "Hola desde subscriber";

    // 1. Crear socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error creando socket");
        exit(EXIT_FAILURE);
    }

    // 2. Configurar dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // localhost
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // 3. Conectar
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en connect");
        exit(EXIT_FAILURE);
    }

    // 4. Enviar mensaje
    send(sock, message, strlen(message), 0);
    printf("Mensaje enviado\n");

    close(sock);

    return 0;
}