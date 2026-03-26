#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 5000 // Puerto en el que el broker estará escuchando
#define DIRECCION "127.0.0.1" // Dirección del broker (localhost en este caso)
#define TAMANO 256

main(int argc, char *argv[])
{
    // Variables para el socket, mensaje, contenido y topic
    int sock;
    char mensaje[TAMANO];
    char contenido[TAMANO];
    char *topic;

    // Estructura para la dirección del broker
    struct sockaddr_in ser_adr;

    system("clear");

    printf("APLICACION PUBLISHER -- Corresponsal deportivo de AndeSports\n");

    // Validar argumento (topic)
    if (argc < 2)
    {
        printf("Uso: %s <topic>\n", argv[0]);
        exit(-1);
    }

    // Obtener topic del argumento de línea de comandos
    topic = argv[1];

    
    printf("Partido seleccionado: %s\n", topic);

    printf("\nSe va a ejecutar socket(AF_INET, SOCK_STREAM, 0)\n");
    printf("Pulse <enter> para continuar...\n");
    getchar();

    // 1. Crear socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Creando Socket");
        exit(-1);
    }

    // 2. Configurar dirección del broker
    ser_adr.sin_family = AF_INET;
    ser_adr.sin_addr.s_addr = inet_addr(DIRECCION);
    ser_adr.sin_port = htons(PORT);

    printf("\nSe va a ejecutar connect()\n");
    printf("Pulse <enter> para continuar...\n");
    getchar();

    // 3. Conectar al broker
    if (connect(sock, (struct sockaddr*)&ser_adr, sizeof(ser_adr)) == -1)
    {
        perror("Connect");
        exit(-1);
    }

    printf("Corresponsal conectado al servidor\n");

    // 4. Loop de envío de eventos
    while (1)
    {
        printf("\nIngrese evento (ej: Gol minuto 32): ");
        // Leer evento desde la entrada estándar
        fgets(contenido, TAMANO, stdin);

        // eliminar salto de línea
        contenido[strcspn(contenido, "\n")] = 0;

        // construir mensaje tipo PUBLISH
        sprintf(mensaje, "PUBLISH|%s|%s", topic, contenido);

        printf("\nSe va a ejecutar send()\n");
        printf("Mensaje a enviar: %s\n", mensaje);
        printf("Pulse <enter> para continuar...\n");
        getchar();

        // enviar mensaje
        if (send(sock, mensaje, strlen(mensaje), 0) == -1)
        {
            perror("send");
            exit(-1);
        }

        printf("Mensaje enviado correctamente\n");
    }

    close(sock);
}