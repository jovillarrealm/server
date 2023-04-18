#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>


#include "showArchivos.c"
#include "showArchivos.h"
#include "saveArchivos.h"
#include "saveArchivos.c"

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 15
#define MAX_REQUEST_SIZE 4096
#define MAX_RESPONSE_SIZE 4096
#define OK 200
#define BAD_REQUEST 400
#define NOT_FOUND 404

#include "http_request.h"

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
        // Trata de obtener host desde antes

        char *host_start = strstr(method_end, "Host: ");
        if (host_start)
        {
            host_start += 6;
            char *host_end = strstr(host_start, "\r\n");
            size_t host_len = host_end - host_start;
            request->host = strndup(host_start, host_len);
        }
        else
        {
            request->host = strdup("");
        }
        request->path = strdup(uri);
    }
    printf("->> Host: %s\n", request->host);
    printf("->> Ruta: %s\n", request->path);
    free(uri);

    if (request->method == POST)
    {
        char *content_type_start = strstr(method_end, "Content-Type: ");
        if (content_type_start != NULL)
        {
            content_type_start += 14;
            char *content_type_end = strstr(content_type_start, "\r\n");
            size_t content_type_len = content_type_end - content_type_start;
            request->content_type = strndup(content_type_start, content_type_len);

            // Detectar el tipo de archivo y asignar la extensión correspondiente
            if (strncmp(request->content_type, "text/plain", 10) == 0) {
                request->file_ext = strdup(".txt");
            } else if (strncmp(request->content_type, "image/png", 9) == 0) {
                request->file_ext = strdup(".png");
            } else if (strncmp(request->content_type, "image/jpeg", 9) == 0) {
                request->file_ext = strdup(".jpeg");
            } else if (strncmp(request->content_type, "image/gif", 15) == 0) {
                request->file_ext = strdup(".gif");
            } else if (strncmp(request->content_type, "application/pdf", 15) == 0) {
                request->file_ext = strdup(".pdf");
            } else if (strncmp(request->content_type, "application/octet-stream", 24) == 0) {
                request->file_ext = strdup(".exe");
            } else {
                request->file_ext = strdup(".dat");
            }
        }

        char *content_length_start = strstr(method_end, "Content-Length: ");
        if (content_length_start != NULL)
        {
            content_length_start += 16; // saltar los caracteres de "Content-Length:"
            char *content_length_end = strstr(content_length_start, "\r\n");
            size_t content_length_len = content_length_end - content_length_start;
            char content_length_str[content_length_len + 1];
            strncpy(content_length_str, content_length_start, content_length_len);
            content_length_str[content_length_len] = '\0';
            request->content_len = strtol(content_length_str, NULL, 10);
        }
        

        char *body_start = strstr(buffer, "\r\n\r\n");
        if (body_start != NULL) 
        {
            body_start += 4; // saltar los caracteres de separación
            size_t buffer_len = strlen(buffer);
            size_t body_len = buffer + buffer_len - body_start - 4;
            request->body = (char*) malloc(body_len + 1); // incluir espacio adicional para NULL
            memcpy(request->body, body_start, body_len);
            request->body[body_len] = '\0'; // agregar byte NULL

            // Generar el nombre del archivo a partir de la fecha y hora
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            char filename[100];
            sprintf(filename, "%04d-%02d-%02d_%02d-%02d-%02d%s", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, request->file_ext);


           // Guardar el archivo en disco
            int fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
            //write(fd, request->body, request->content_len);
            write(fd, request->body, body_len);
            close(fd);
        }
        printf("->> Body: %s\n", request->body);
    }
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
    // printf("Solicitud HTTP recibida: %s\n", request_buffer);

    // Analizar la línea de solicitud HTTP
    http_request request;
    parse_request_line(request_buffer, &request);
    prequest(request);
    logger(request.path, log_file);

    // Determinar el estado de la solicitud y generar una respuesta HTTP
    char *response_body = NULL;
    size_t response_length = 0;
    int response_code = 0;
    char *status_text = NULL;

    switch (request.method)
    {
    case GET:
        printf("lets do a get! \n");
        char *path = memmove(request.path, request.path + 1, strlen(request.path));
        showFile(PORT, client_fd, path);
        break;
    case POST:
        printf("lets do a post! \n");
        saveFile(&request);

        response_code = 200;
        status_text = "OK";
        response_body = "<html><body><h1>¡Gracias por enviar datos, jeje!</h1></body></html>";
        response_length = strlen(response_body);
        break;
    default:
        printf("Oh no, bad request! \n");
        response_code = 400;
        status_text = "Bad Request";
        response_body = "<html><body><h1>Solicitud HTTP no válida, ayudame cristooo</h1></body></html>";
        response_length = strlen(response_body);
        break;
    }

    close(client_fd);
}

// Función para el hilo que acepta conexiones de clientes
void *accept_connections(void *server_fd_ptr)
{
    // cambio para consistencia con logger
    connection_info thread_info = *(connection_info *)server_fd_ptr;
    int server_fd = *(int *)server_fd_ptr;
    FILE *log_file = thread_info.log_file;
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0)
        {
            error("Error al aceptar la conexión del cliente");
        }
        handle_connection(client_fd, log_file);
    }
    return NULL;
}

// Funcion para manejar conexiones de varios clientes

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
        log_file = fopen("log.txt", "a+");
        doc_root_folder = " ./logs";
    }

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

        while (1)
    {
        // Acepta una nueva conexión entrante
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Error al aceptar la conexión entrante \n");
            continue;
        }
        handle_connection(client_fd, log_file);
    }
    fclose(log_file); // Unreachable en este momento
    return 0;
}