#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 15
#define MAX_REQUEST_SIZE 4096
#define MAX_RESPONSE_SIZE 4096
#define OK 200
#define BAD_REQUEST 400
#define NOT_FOUND 404

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
    printf("->> Ruta: %s\n", request->path);
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
    char *response_body = NULL;
    size_t response_length = 0;
    int response_code = 0;
    char *status_text = NULL;

    switch (request.method)
    {
    case GET:
        response_code = 200;
        status_text = "OK";
        response_body = "<html><body><h1>¡Hola, mundo! Si, Funciono</h1></body></html>";
        response_length = strlen(response_body);
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

    // Generar la respuesta HTTP
    char response_buffer[MAX_RESPONSE_SIZE];
    snprintf(response_buffer, MAX_RESPONSE_SIZE,
             "%s %d %s\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %zu\r\n"
             "Host: localhost\r\n"
             "Date: %s\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             (strcmp(request.version, "HTTP/1.1") == 0) ? "HTTP/1.1" : "HTTP/1.0", response_code, status_text, response_length, get_current_time(), response_body);
    int response_length_written = snprintf(response_buffer, MAX_RESPONSE_SIZE, "%s %d %s\r\nContent-Length: %lu\r\nContent-Type: text/html\r\n\r\n%s", request.version, response_code, status_text, response_length, response_body);
    if (response_length_written >= MAX_RESPONSE_SIZE)
    {
        error("Respuesta HTTP demasiado larga para el búfer");
    }

    // Enviar la respuesta HTTP al cliente
    ssize_t bytes_sent = send(client_fd, response_buffer, strlen(response_buffer), 0);
    if (bytes_sent < 0)
    {
        error("Error al enviar la respuesta HTTP");
    }
    logger(response_buffer, log_file);
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
    int errno = 0;
    long val = strtol(str, &end, 10);
    if (errno || end == str || *end != '\0' || val < 0 || val >= 0x10000)
    {
        return -1;
    }
    *res = (uint16_t)val;
    return 0;
}

static volatile sig_atomic_t running = 1;
void ensure_good_exit(int _ctr_c)
{
    (void)_ctr_c;
    printf("Salimos bien\n");
    running = 0;
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
        port = 8080;
        log_file = fopen("./logs/log.txt", "a+");
        doc_root_folder = "./assets";
    }

    if (signal(SIGINT, ensure_good_exit) == SIG_ERR)
    printf("\ncan't catch SIGINT\n");

    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Crea el socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Error al crear el socket \n");
        exit(EXIT_FAILURE);
    }

    // Configura la dirección y el puerto para el socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Fix de "address already in use"
    int _ = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &_, sizeof(_));

    // Vincula el socket al puerto y la dirección especificados
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Error al vincular el socket \n");
        exit(EXIT_FAILURE);
    }

    // Escucha las conexiones entrantes
    if (listen(server_fd, MAX_CONNECTIONS) < 0)
    {
        perror("Error al escuchar en el socket \n");
        exit(EXIT_FAILURE);
    }
    printf("Usando assets de %s\n", doc_root_folder);
    printf("Servidor iniciado en el puerto %d...\n", port);

    while (running)
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
    fclose(log_file); // Unreachable en este momento
    return 0;
}
