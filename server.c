#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include "showArchivos.h"


#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 15
#define MAX_REQUEST_SIZE 4096
#define MAX_RESPONSE_SIZE 4096
#define OK 200
#define BAD_REQUEST 400
#define NOT_FOUND 404
// Evil
#define MAX_EVENTS 128
#define MAX_THREADS 2
#define QUEUE_SIZE 256

// Define la estructura de la petición HTTP:
typedef struct http_request
{
    enum method
    {
        GET,
        POST,
        PUT,
        DELETE,
        UNSUPPORTED
    } method;
    char *host;
    char *path;
    char version[16];
    int status_code;
} http_request;

// Define la estructura para pasar a los hilos de cada conexión
typedef struct connection_info
{
    int client_fd;
    FILE *log_file;
} connection_info;

// Para el manejo del estado del servidor
typedef struct ServerState
{
    int epoll_fd;
    int listen_fd;
    int queue[QUEUE_SIZE];
    int queue_front;
    int queue_rear;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    FILE *log;
} ServerState;

// Aux para saber el tiempo exactamente
char *get_current_time()
{
    time_t t = time(NULL);
    struct tm tm = *gmtime(&t);
    char *time_str = malloc(30 * sizeof(char));
    strftime(time_str, 30, "%a, %d %b %Y %H:%M:%S GMT", &tm);
    return time_str;
}

// Muestra un mensaje de error y sale del programa
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

// Logger
void logger(const char *message, FILE *log_file)
{
    time_t rawtime;
    struct tm *timeinfo;
    char time_str[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%d-%m-%Y %H:%M:%S", timeinfo);

    fprintf(log_file, "[%s] %s\n\n", time_str, message);
    fflush(log_file);
}

// Analiza la línea de solicitud HTTP para determinar el método, la ruta y el host
void parse_request_line(char *buffer, http_request *request)
{
    char *method_end = strchr(buffer, ' ');
    if (method_end == NULL)
    {
        request->method = UNSUPPORTED;
        return;
    }
    size_t method_len = method_end - buffer;
    if (strncmp(buffer, "GET", method_len) == 0)
    {
        request->method = GET;
    }
    else if (strncmp(buffer, "POST", method_len) == 0)
    {
        request->method = POST;
    }
    else if (strncmp(buffer, "PUT", method_len) == 0)
    {
        request->method = PUT;
    }
    else if (strncmp(buffer, "DELETE", method_len) == 0)
    {
        request->method = DELETE;
    }
    else
    {
        request->method = UNSUPPORTED;
    }
    printf("--> Método HTTP: %d\n", request->method);

    char *uri_start = method_end + 1;
    char *uri_end = strchr(uri_start, ' ');
    if (uri_end == NULL)
    {
        request->method = UNSUPPORTED;
        return;
    }
    size_t uri_len = uri_end - uri_start;
    char *http_version_start = uri_end + 1;
    if (http_version_start[0] != 'H' || http_version_start[1] != 'T' || http_version_start[2] != 'T' || http_version_start[3] != 'P' || http_version_start[4] != '/')
    {
        request->method = UNSUPPORTED;
        printf("-->http_version_start no comenza con HTTP/. ");
        return;
    }
    if (http_version_start[5] == '0')
    {
        strcpy(request->version, "HTTP/1.0");
        printf("-->HTTP version 1.0 detectado. \n");
    }
    else if (http_version_start[5] == '1')
    {
        strcpy(request->version, "HTTP/1.1 ");
        printf("-->HTTP version 1.1 detectado. \n");
    }
    else
    {
        request->method = UNSUPPORTED;
        printf("--> No se puede detectar la version de HTTP o no esta soportado. \n");
        return;
    }
    request->version[7] = '\0';
    char *uri = strndup(uri_start, uri_len);
    char *host_start = strstr(uri, "://");
    if (host_start)
    {
        host_start += 3;
        char *path_start = strchr(host_start, '/');
        if (path_start)
        {
            *path_start = '\0';
            path_start++;
            request->host = strdup(host_start);
            request->path = strdup(path_start);
        }
        else
        {
            request->host = strdup(host_start);
            request->path = strdup("");
        }
    }
    else
    {
        request->host = strdup("");
        request->path = strdup(uri);
    }
    printf("->> Host: %s\n", request->host);
    printf("->> Ruta: %s\n\n", request->path);
    free(uri);
}

// Función para manejar una conexión de cliente
void handle_connection(int client_fd, FILE *log_file)
{
    char request_buffer[MAX_REQUEST_SIZE];
    ssize_t bytes_received = recv(client_fd, request_buffer, MAX_REQUEST_SIZE - 1, 0);
    if (bytes_received < 0)
    {
        // FIXME El servidor debería tratar de retornar un BAD REQUEST?
        perror("Error al recibir la solicitud HTTP");
        return;
    }
    request_buffer[bytes_received] = '\0';
    logger(request_buffer, log_file);
    printf("Solicitud HTTP recibida: %s\n", request_buffer);

    // Analizar la línea de solicitud HTTP
    http_request request;
    parse_request_line(request_buffer, &request);
    logger(request.path, log_file);

    // Determinar el estado de la solicitud y generar una respuesta HTTP
    char *response_body = "<html><body><h1>Solicitud HTTP no válida, ayudame cristooo</h1></body></html>";
    size_t response_length = strlen(response_body);
    int response_code;
    char *status_text = "Bad Request";

    switch (request.method)
    {
    case GET:
        memmove(request.path, request.path + 1, strlen(request.path));
        showFile(client_fd, request.path);
        break;
    case POST:
        response_code = 200;
        status_text = "OK";
        response_body = "<html><body><h1>¡Gracias por enviar datos, jeje!</h1></body></html>";
        response_length = strlen(response_body);
        break;
    default:
        response_code = 400;
        status_text = "Bad Request";
        response_body = "<html><body><h1>Solicitud HTTP no válida, ayudame cristooo</h1></body></html>";
        response_length = strlen(response_body);
        break;
    }

    close(client_fd);
}

// Funcion para manejar conexiones de varios clientes
void *connection_handler(void *arg)
{
    connection_info thread_info = *(connection_info *)arg;
    int client_fd = thread_info.client_fd;
    FILE *log_file = thread_info.log_file;
    handle_connection(client_fd, log_file);
    return NULL;
}

// De: https://stackoverflow.com/questions/20019786/safe-and-portable-way-to-convert-a-char-to-uint16-t
int str_to_uint16(char *str, uint16_t *res)
{
    char *end;
    // si errno da problemas eliminarlo aquí
    long val = strtol(str, &end, 10);
    if (errno || end == str || *end != '\0' || val < 0 || val >= 0x10000)
    {
        return -1;
    }
    *res = (uint16_t)val;
    return 0;
}

int listening_socket(u_int16_t port)
{
    // Crea el socket
    int server_fd;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Error al crear el socket \n");
        exit(EXIT_FAILURE);
    }

    // Configura la dirección y el puerto para el socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Fix de "address already in use"
    int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // Vincula el socket al puerto y la dirección especificados
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Error al vincular el socket \n");
        exit(EXIT_FAILURE);
    }

    // set the socket to non-blocking mode
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    // Escucha las conexiones entrantes
    if (listen(server_fd, MAX_CONNECTIONS) < 0)
    {
        perror("Error al escuchar en el socket \n");
        exit(EXIT_FAILURE);
    }
    return server_fd;
}

// Hilo que lockea
void *worker_thread(void *arg)
{
    ServerState *state = (ServerState *)arg;

    while (1)
    {
        // Espera disponibilidad
        pthread_mutex_lock(&state->mutex);

        while (state->queue_front == state->queue_rear)
        {
            pthread_cond_wait(&state->cond, &state->mutex);
        }

        // Se agrega a la cola del estado y actualiza
        int client_fd = state->queue[state->queue_front];
        state->queue_front = (state->queue_front + 1) % QUEUE_SIZE; // esto es lo que revisa el main thread

        pthread_mutex_unlock(&state->mutex);

        // Ahora se maneja la conexion

        handle_connection(client_fd, state->log);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    // Del llamado de programa

    // Puerto que va a escuchar el servidor
    // Rúbrica: HTTP PORT es el puerto de la máquina en la cual está corriendo su servidor web TWS
    u_int16_t port;

    // FILE * en modo a+ para solo tener append en el log file
    // LogFile es el archivo de log que se va a generar con toda la información concerniente a peticiones, errores, info, etc.
    FILE *log_file;

    // DocumentRootFolder es la carpeta donde se alojarán los diferentes recursos web
    char *doc_root_folder;
    // Si no se da el uso de la rúbrica al programa, usa unos datos por defecto
    if (argc == 4)
    {
        printf("uso: ./server <PORT> <LOG_FILE> <DOC_ROOT_FOLDER>\n");
        if (str_to_uint16(argv[1], &port) == -1)
        {
            exit(EXIT_FAILURE);
        }

        log_file = fopen(argv[2], "a+");
        doc_root_folder = argv[3];
    }
    else
    {
        port = 8080;
        log_file = fopen("./logs/log.txt", "a+");
        doc_root_folder = "./docs";
    }

    // FIXME: Si accept no tiene problemas, esto se borra.
    
    // int addrlen = sizeof(address);

    int server_fd = listening_socket(port);

    // Se instancia un epoll para el manejo óptimo de conexiones

    // fd de epoll
    int epoll_fd;
    if ((epoll_fd = epoll_create1(0)) == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    // Se configura el epoll

    // Evento asociado al fd del server
    // No se va a definir EPOLLET por lo que estaremos trabajando en Level-
    struct epoll_event event = {
        .events = EPOLLIN,
        .data.fd = server_fd,
    };
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1)
    {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    // El estado del servidor, para evitar el uso de globales
    ServerState state = {
        .epoll_fd = epoll_fd,
        .listen_fd = server_fd,
        .queue_front = 0,
        .queue_rear = 0,
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .cond = PTHREAD_COND_INITIALIZER,
        .log = log_file,
    };

    printf("Usando assets de %s\n", doc_root_folder);
    printf("Servidor iniciado en el puerto %d...\n", port);

    // pthread_t[] para el manejo de cada conexión. A cada hilo se le va a pasar el estado del servidor
    // Threadpool Deadpool para I/O Multiplex multiflex, cool.
    pthread_t client_threads[MAX_THREADS];
    for (int thread_id = 0; thread_id < MAX_THREADS; thread_id++)
    {
        if (pthread_create(&client_threads[thread_id], NULL, worker_thread, &state) != 0)
        {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    // while loop donde se reciben y lidian conexiones
    struct epoll_event events[MAX_EVENTS];
    while (1)
    {
        // Numero de eventos que buscan
        int triggered_event_number = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (triggered_event_number == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                perror("epoll_wait");
                exit(EXIT_FAILURE);
            }
        }

        // Por cada evento que haya pasado, se le acepta conexión y se agrega al estado del servidor

        for (int event_i = 0; event_i < triggered_event_number; event_i++)
        {
            if (events[event_i].data.fd == server_fd)
            {
                // accept() con más error handling
                // FIXME: posiblemente error de accept si era  if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
                // FIXME: posiblemente error de accept si era  if ((client_fd = accept(server_fd, NULL, NULL)) < 0)
                while (1)
                {
                    // Se acepta conexion
                    int client_fd = 0;
                    if ((client_fd = accept(server_fd, NULL, NULL)) < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            break;
                        }
                        else
                        {
                            perror("accept");
                            exit(EXIT_FAILURE);
                        }
                    }

                    // Para agregar la conexion al estado, la debe meter primero a epoll

                    // Para meterla, la conexion se configura el fd como no block-eante
                    int flags = fcntl(client_fd, F_GETFL, 0);
                    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

                    // ...y se reconfigura epoll a través de un evento asociado a la conexion
                    struct epoll_event client_event = {
                        .events = EPOLLIN | EPOLLET,
                        .data.fd = client_fd,
                    };
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) == -1)
                    {
                        perror("epoll_ctl");
                        exit(EXIT_FAILURE);
                    }

                    // Encola al cliente para ser manejado por uno de los pthread_t
                    // FIXME: Este código exacto está repetido más abajo, aún está por determinar por qué
                    // de pronto esto solo ocurre la primera vez pero no se alteran los events[i].data.fd

                    pthread_mutex_lock(&state.mutex);
                    // Mientras los hilos están ocupados, main thread espera
                    while ((state.queue_rear + 1) % QUEUE_SIZE == state.queue_front)
                    {
                        pthread_cond_wait(&state.cond, &state.mutex);
                    }

                    state.queue[state.queue_rear] = client_fd;
                    state.queue_rear = (state.queue_rear + 1) % QUEUE_SIZE;

                    pthread_mutex_unlock(&state.mutex);
                    pthread_cond_signal(&state.cond);
                }
            }
            else
            {
                // Encola al cliente para ser manejado por uno de los pthread_t
                pthread_mutex_lock(&state.mutex);

                while ((state.queue_rear + 1) % QUEUE_SIZE == state.queue_front)
                {
                    pthread_cond_wait(&state.cond, &state.mutex);
                }

                state.queue[state.queue_rear] = events[event_i].data.fd;
                state.queue_rear = (state.queue_rear + 1) % QUEUE_SIZE;

                pthread_mutex_unlock(&state.mutex);
                pthread_cond_signal(&state.cond);
            }
        }
    }

    return 0;
}

int old_main(FILE *log_file)
{
    /*
        while (1)
        {
            // Acepta una nueva conexión entrante
            if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("Error al aceptar la conexión entrante \n");
                continue;
            }

            // Crea un nuevo hilo para manejar la conexión entrante
            pthread_t thread_id;
            connection_info thread_info = {.client_fd = client_fd, .log_file = log_file};
            if (pthread_create(&thread_id, NULL, connection_handler, (void *)&thread_info) < 0)
            {
                perror("Error al crear el hilo \n");
                continue;
            }

            // El hilo manejará la conexión, así que podemos continuar aceptando conexiones entrantes
            pthread_detach(thread_id);
        }
    */

    fclose(log_file); // Unreachable en este momento
    return 0;
}
