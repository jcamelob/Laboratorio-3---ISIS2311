#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
//#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

#define PORT 5000
#define TAMANO 256

int main() {

    //Creamos 2 sockets: uno para escuchar (sock) y otro para aceptar conexiones (sock2)
    int sock, sock2;
    // Estructuras para la dirección del servidor y del cliente
    struct sockaddr_in ser_adr, cli_adr;
    // Variable para el tamaño de la dirección del cliente
    socklen_t lg;
    // Buffer para almacenar el mensaje recibido
    char mensaje[TAMANO];
    // Variable para el PID del proceso hijo
    int pid;

    printf("BROKER TCP INICIADO --- AndeSports \n");

    // 1. Crear socket
    //AF_INET: IPv4, SOCK_STREAM: TCP, 0: protocolo por defecto
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Error creando socket");
        exit(EXIT_FAILURE);
    }

    // 2. Configurar dirección del servidor
    ser_adr.sin_family = AF_INET; // IPv4
    ser_adr.sin_addr.s_addr = INADDR_ANY;  // Aceptar conexiones en cualquier interfaz
    ser_adr.sin_port = htons(PORT); // Convertir el puerto a formato de red

    // 3. Bind
    // Asociar el socket con la dirección y puerto configurados
    if (bind(sock, (struct sockaddr*)&ser_adr, sizeof(ser_adr)) == -1) {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }

    // 4. Listen
    // Poner el socket en modo escucha para aceptar conexiones entrantes (5 es el número máximo de conexiones en cola)
    if (listen(sock, 5) == -1) {
        perror("Error en listen");
        exit(EXIT_FAILURE);
    }

    printf("Escuchando en puerto %d...\n", PORT);

    // 5. Loop principal
    // El broker se queda en un loop infinito esperando conexiones de clientes (publishers o subscribers)
    while (1) {

        // Obtener el tamaño de la dirección del cliente
        lg = sizeof(cli_adr);

        // Accept
        // Esperar a que un cliente se conecte y aceptar la conexión, obteniendo un nuevo socket (sock2) para comunicarse con ese cliente
        sock2 = accept(sock, (struct sockaddr*)&cli_adr, &lg);
        while (waitpid(-1, NULL, WNOHANG) > 0);
        if (sock2 == -1) {
            perror("Error en accept");
            continue;
        }

        // Fork para concurrencia
        pid = fork();

        if (pid == -1) {
            perror("Error en fork");
            continue;
        }

        // ===== PROCESO HIJO =====
        if (pid == 0) {

            close(sock); // el hijo no usa el socket principal

            printf("\nCliente conectado\n");

            while (1) {

                memset(mensaje, 0, TAMANO);

                int bytes = recv(sock2, mensaje, TAMANO, 0);

                // Si el cliente se desconecta
                if (bytes <= 0) {
                    printf("Cliente desconectado\n");
                    break;
                }

                printf("Mensaje bruto recibido:\n%s\n", mensaje);

                // ===== PARSING =====
                char *tipo = strtok(mensaje, "|");
                char *topic = strtok(NULL, "|");
                char *contenido = strtok(NULL, "|");

                printf("----- Parsing -----\n");

                if (tipo != NULL)
                    printf("Tipo: %s\n", tipo);
                else
                    printf("Tipo: NULL\n");

                if (topic != NULL)
                    printf("Topic: %s\n", topic);
                else
                    printf("Topic: NULL\n");

                if (contenido != NULL)
                    printf("Contenido: %s\n", contenido);
                else
                    printf("Contenido: NULL\n");

                printf("-------------------\n");
            }

            close(sock2);
            exit(0);
        }

        // ===== PROCESO PADRE =====
        close(sock2);
    }

    return 0;
}