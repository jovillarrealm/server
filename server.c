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
#include "http_request.h"
#include "http_request.c"
#include "thread_utils.h"
#include "showArchivos.h"
#include "showArchivos.c"
#include "saveArchivos.h"
#include "saveArchivos.c"
#include "showHeaders.h"
#include "showHeaders.c"

// Funcion para manejar conexiones de varios clientes

// De: https://stackoverflow.com/questions/20019786/safe-and-portable-way-to-convert-a-char-to-uint16-t
int str_to_uint16(char *str, uint16_t *res)
{
    char *end;
    long val = strtol(str, &end, 10);
    if (end == str || *end != '\0' || val < 0 || val >= 0x10000)
    {
        return -1;
    }
    *res = (uint16_t)val;
    return 0;
}

int main(int argc, char *argv[])
{
    // Del llamado de programa
    u_int16_t port;
    FILE *log_file;
    char *doc_root_folder;
    // Si se llama el programa correctamente, usa esos datos
    if (argc == 4)
    {
        if (str_to_uint16(argv[1], &port) == -1)
        {
            perror("uso: ./server <PORT> <LOG_FILE> <DOC_ROOT_FOLDER>\n");
            exit(EXIT_FAILURE);
        }
        log_file = fopen(argv[2], "a+"); // a+ (create + append) option will allow appending which is useful in a log file
        doc_root_folder = argv[3];
    }
    else
    {
        perror("uso: ./server <PORT> <LOG_FILE> <DOC_ROOT_FOLDER>\n");
        port = 8080;
        log_file = fopen("log.txt", "a+");
        doc_root_folder = "assets";
    }

    int client_fd;
    // Configura la dirección y el puerto para el socket

    struct sockaddr_in address =
        {
            .sin_family = AF_INET,
            .sin_addr.s_addr = INADDR_ANY,
            .sin_port = htons(port),
        };
    int addrlen = sizeof(address);
    int server_fd = listening_socket(port, address);

    // Se instancia un epoll para el manejo óptimo de conexiones

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
        .doc_root = doc_root_folder,
    };

    // Threadpool Deadpool para I/O Multiplex multiflex. A cada hilo se le va a pasar el estado del servidor
    pthread_t client_threads[MAX_THREADS];
    for (int thread_id = 0; thread_id < MAX_THREADS; thread_id++)
    {
        if (pthread_create(&client_threads[thread_id], NULL, worker_thread, &state) != 0)
        {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    printf("Servidor iniciado en el puerto %d...\n", port);
    struct epoll_event events[MAX_EVENTS];
    while (1)
    {
        // Numero de eventos que tuvieron actividad hasta ahora
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
                // accept() con mas error handling
                // FIXME: posiblemente error de accept si era  if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
                // FIXME: posiblemente error de accept si era  if ((client_fd = accept(server_fd, NULL, NULL)) < 0)
                while (1)
                {
                    // Se acepta conexion
                    int client_fd = -1;
                    if ((client_fd = accept(server_fd, NULL, NULL)) < 0)
                    {
                        printf("acepta conexion\n");
                        // manejo para que cuando esté non blocking no se explote
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            break;
                        }
                        else
                        { 

                            perror("accept");
                            exit(EXIT_FAILURE);
                        }
                    } else
                    {
                        
                    
                    

                    // Para agregar la conexion al estado, la debe meter primero a epoll

                    // Para meterla, la conexion se configura el fd como no block-eante

                    int flags = fcntl(client_fd, F_GETFL, 0);
                    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

                    // ...y se reconfigura epoll a traves de un evento asociado a la conexion
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
                    // FIXME: Este código exacto esta repetido mas abajo, aún esta por determinar por que
                    // de pronto esto solo ocurre la primera vez pero no se alteran los events[i].data.fd

                    pthread_mutex_lock(&state.mutex);
                    // Mientras los hilos estan ocupados, main thread espera
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
    /*
        // Acepta una nueva conexión entrante

        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Error al aceptar la conexión entrante \n");
            continue;
        }
        handle_connection(client_fd, log_file, doc_root_folder);
    }
    fclose(log_file); // Unreachable en este momento
    */
    return 0;
}

// Función para manejar una conexión de cliente
void handle_connection(int client_fd, FILE *log_file, char *doc_root)
{

    void *request__buff = malloc(MAX_REQUEST_SIZE); // max TCP size 65535
    ssize_t bytes_received = 0;
    while (bytes_received = recv(client_fd, request__buff, MAX_REQUEST_SIZE, 0) < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            continue;
        }
        else
        {
            if (bytes_received < 0)
            {
                perror("Error al recibir la solicitud HTTP");
                free(request__buff);
                return;
            }
        }
    }

    // Analizar la línea de solicitud HTTP
    http_request request = {.method = 5, .body = NULL, .body_size = 0, .content_len = 0, .content_type = NULL, .host = NULL, .path = NULL, .status_code = 400, .version = "               ", .header_size = 0, .doc_root_folder = doc_root};
    char *status_headers = parse_request(request__buff, bytes_received, &request, doc_root, client_fd);
    logger(status_headers, log_file);

    prequest(&request);

    // Determinar el estado de la solicitud y generar una respuesta HTTP

    switch (request.method)
    {
    case GET:
        printf("lets do a GET! \n");
        showFile(client_fd, request.path);
        break;
    case POST:
        printf("lets do a POST! \n");
        saveFile(&request, client_fd);
        break;
    case HEAD:
        printf("lets return a HEAD! \n");
        showHeaders(client_fd, request.path);
        break;
    default:
        printf("Oh no, bad request! \n");
        free(request.body);
        break;
    }

    close(client_fd);
}

// EVIL bitchcraft Do nOt eaT
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

        handle_connection(client_fd, state->log, state->doc_root);
    }

    return NULL;
}
