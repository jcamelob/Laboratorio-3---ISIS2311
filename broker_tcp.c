#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> //htons() y inet_addr()
#include <sys/select.h> // Para usar select() y fd_set, FD_SET, FD_ZERO, etc.

#define PORT 5000 // Puerto en el que el broker estará escuchando (arbitrario)
#define TAMANO 256
#define MAX_CLIENTES 10
#define MAX_SUBS 20

// Estructura para subscribers, con ella podemos saber para cada subscriber a qué topic está suscrito y su socket para enviarle eventos
typedef struct {
    int socket;
    char topic[50];
} Subscriber;

// Lista de subscribers
Subscriber subs[MAX_SUBS];
int subs_count = 0;

int main() {

    // Variables para el servidor, nuevo socket y clientes (array de sockets)
    int servidor, nuevo_socket, clientes[MAX_CLIENTES];

    // Estructura para la dirección del broker, guarda la Ip y el puerto
    struct sockaddr_in address;

    // Longitud de la dirección, se usa para la función accept() que espera un puntero a esta variable para llenarla con la información del cliente que se conecta
    int addrlen = sizeof(address);

    // Buffer para recibir mensajes de los clientes
    char buffer[TAMANO];

    // Conjunto de descriptores para select(), representa el conjunto de sockets que queremos monitorear para ver si tienen actividad (nueva conexión o mensaje entrante)
    fd_set readfds;

    // Inicializar clientes
    for (int i = 0; i < MAX_CLIENTES; i++)
        clientes[i] = 0; // 0 indica que el espacio está libre

    // 1. Crear socket servidor
    //AF_INET: IPv4, SOCK_STREAM: TCP, 0: protocolo por defecto
    servidor = socket(AF_INET, SOCK_STREAM, 0);

    if (servidor == -1) {
        perror("Error creando socket");
        exit(-1);
    }

    printf("BROKER TCP INICIADO --- AndeSports \n");

    // 2. Configurar socket para reutilizar dirección (evitar error "Address already in use" al reiniciar el servidor rápidamente)
    int opt = 1;
    setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //3. Configurar direccion del broker
    address.sin_family = AF_INET; // IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Aceptar conexiones en cualquier interfaz de red
    address.sin_port = htons(PORT); // Convertir puerto a formato de red


    // 4. Bind
    //Asociar el socket a la dirección y puerto configurados
    if (bind(servidor, (struct sockaddr*)&address, sizeof(address)) == -1) {
        perror("Error en bind");
        exit(-1);
    }
    //5. Listen
    //Poner el socket en modo escucha para aceptar conexiones entrantes, el número 5 es el tamaño de la cola de conexiones pendientes
    if (listen(servidor, 5) == -1) {
        perror("Error en listen");
        exit(-1);
    }

    printf("BROKER iniciado en puerto %d\n", PORT);

    // 6. Loop principal 
    // El broker se queda en un loop infinito esperando conexiones de clientes (publishers o subscribers)
    while (1) {

        // Limpiar el conjunto de sockets que estoy vigilando
        FD_ZERO(&readfds);

        // Agregar socket servidor
        FD_SET(servidor, &readfds); 
        int max_sd = servidor; // El descriptor máximo, necesario para la función select(), inicialmente es el socket del servidor

        // Agregar clientes
        for (int i = 0; i < MAX_CLIENTES; i++) {
            int sd = clientes[i]; // Obtener el socket del cliente

            if (sd > 0) // Si el socket es válido (mayor que 0)
                FD_SET(sd, &readfds); // Agregar el socket del cliente al conjunto de sockets a monitorear

            if (sd > max_sd) // Actualizar el descriptor máximo si el socket del cliente es mayor
                max_sd = sd;
        }

        // Esperar actividad
        //La función select() bloquea hasta que hay actividad en alguno de los sockets del conjunto readfds, 
        //el primer argumento es el descriptor máximo + 1, 
        //el segundo argumento es el conjunto de sockets a monitorear para lectura, 
        //los otros dos NULL indican que no estamos monitoreando para escritura ni para excepciones, y el último NULL indica que no hay un timeout (espera indefinidamente)
        select(max_sd + 1, &readfds, NULL, NULL, NULL);

        // NUEVA CONEXION
        // Si el socket del servidor tiene actividad, significa que hay una nueva conexión entrante
        //FD_ISSET verifica si un socket específico tiene actividad (en este caso, el socket del servidor)
        if (FD_ISSET(servidor, &readfds)) {

            //Aceptar la nueva conexión, accept() devuelve un nuevo socket que se usará para comunicarse con el cliente que se acaba de conectar, 
            //y llena la estructura address con la información del cliente
            nuevo_socket = accept(servidor, (struct sockaddr*)&address, (socklen_t*)&addrlen);
            printf("Nuevo cliente conectado\n");

            // Agregar nuevo socket a la lista de clientes
            for (int i = 0; i < MAX_CLIENTES; i++) {
                if (clientes[i] == 0) {
                    clientes[i] = nuevo_socket;
                    break; //Es decir, se busca el primer espacio libre en el array de clientes para guardar el nuevo socket, y luego se sale del loop
                }
            }
        }

        // ACTIVIDAD EN CLIENTES
        // para cada cliente, verificamos si tiene actividad (mensaje entrante)
        for (int i = 0; i < MAX_CLIENTES; i++) {

            int sd = clientes[i];

            // Verificar si el socket del cliente tiene actividad
            if (FD_ISSET(sd, &readfds)) {

                // Leer mensaje del cliente
                int valread = recv(sd, buffer, TAMANO, 0);

                // Si valread es 0 o negativo, significa que el cliente se ha desconectado o hubo un error, por lo que cerramos el socket y lo removemos de la lista de clientes
                if (valread <= 0) {
                    printf("Cliente desconectado\n");
                    close(sd); //cerrar el socket del cliente
                    clientes[i] = 0; //remover el socket del cliente de la lista (marcarlo como 0 para indicar que está libre)
                }
                else {
                    // Si se recibió un mensaje, lo procesamos
                    buffer[valread] = '\0'; //Convertir a string

                    printf("\nMensaje recibido: %s\n", buffer);

                    // ===== PARSING =====
                    // El formato del mensaje es: TIPO|TOPIC|CONTENIDO
                    // Donde TIPO puede ser "SUBSCRIBE" o "PUBLISH"
                    char *tipo = strtok(buffer, "|");
                    char *topic = strtok(NULL, "|");
                    char *contenido = strtok(NULL, "|");

                    // ===== SUBSCRIBE =====
                    // Si el mensaje es una suscripción, agregamos el cliente a la lista de subscribers con el topic al que se quiere suscribir
                    if (strcmp(tipo, "SUBSCRIBE") == 0) {
                        
                        subs[subs_count].socket = sd; // Guardamos el socket del cliente para enviarle eventos en el futuro
                        strcpy(subs[subs_count].topic, topic); // Guardamos el topic al que se suscribió el cliente
                        subs_count++; // Incrementamos el contador de subscribers

                        printf("Subscriber agregado a %s\n", topic);
                    }

                    // ===== PUBLISH =====
                    // Si el mensaje es un evento a publicar, lo enviamos a todos los subscribers que estén suscritos al topic correspondiente
                    if (strcmp(tipo, "PUBLISH") == 0) {

                        printf("Publicando en %s: %s\n", topic, contenido);

                        // Enviar el evento a todos los subscribers suscritos al topic
                        for (int j = 0; j < subs_count; j++) {

                            if (strcmp(subs[j].topic, topic) == 0) {

                                send(subs[j].socket, contenido, strlen(contenido), 0);
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}