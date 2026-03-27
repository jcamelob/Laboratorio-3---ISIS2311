#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5000
#define DIRECCION "127.0.0.1"
#define TAMANO 256

int main(int argc, char *argv[])
{
    // Variables para el socket, mensaje, buffer y topic
    int sock;
    char mensaje[TAMANO];
    char buffer[TAMANO];
    char *topic;

    // Estructura para la dirección del broker
    struct sockaddr_in ser_adr;

    system("clear");

    printf("Subscriber - AndeSports\n");

    // Validar argumento (topic)
    if (argc < 2)
    {
        printf("Uso: %s <topic>\n", argv[0]);
        exit(-1);
    }

    // Obtener topic del argumento de línea de comandos
    topic = argv[1];

    printf("Suscribiéndose a: %s\n", topic);

    // 1. Crear socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // 2. Configurar dirección del broker
    ser_adr.sin_family = AF_INET;
    ser_adr.sin_addr.s_addr = inet_addr(DIRECCION);
    ser_adr.sin_port = htons(PORT);

    // 3. Conectar al broker
    connect(sock, (struct sockaddr*)&ser_adr, sizeof(ser_adr));

    // 4. Enviar mensaje de suscripción al broker con el formato: SUBSCRIBE|TOPIC|
    snprintf(mensaje, TAMANO, "SUBSCRIBE|%s|", topic);
    send(sock, mensaje, strlen(mensaje), 0);

    printf("Esperando eventos...\n");

    //5. Esperar eventos del broker
    while (1)
    {
        // Recibir evento del broker
        int bytes = recv(sock, buffer, TAMANO, 0);

        // Si bytes es 0 o negativo, significa que el broker se ha desconectado o hubo un error, por lo que cerramos el socket y salimos del programa
        if (bytes <= 0) {
            printf("Conexion cerrada\n");
            break;
        }

        // Si se recibió un evento, lo mostramos por pantalla
        buffer[bytes] = '\0';

        printf("\n Evento: %s\n", buffer);
    }

    close(sock);

    return 0;
}