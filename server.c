#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 15
#define MAX_REQUEST_SIZE 4096
#define MAX_RESPONSE_SIZE 4096
#define OK 200
#define BAD_REQUEST 400
#define NOT_FOUND 404


// Define la estructura de la petición HTTP:
typedef struct {
    enum method {GET, POST, PUT, DELETE, UNSUPPORTED} method;
    char *host;
    char *path;
    char version[16];
    int status_code;
} http_request;

// Aux para saber el tiempo exactamente
char *get_current_time() {
    time_t t = time(NULL);
    struct tm tm = *gmtime(&t);
    char *time_str = malloc(30 * sizeof(char));
    strftime(time_str, 30, "%a, %d %b %Y %H:%M:%S GMT", &tm);
    return time_str;
}

// Muestra un mensaje de error y sale del programa
void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Logger
void logger(const char *message) {
    time_t rawtime;
    struct tm *timeinfo;
    char time_str[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%d-%m-%Y %H:%M:%S", timeinfo);

    FILE *log_file = fopen("log.txt", "a");
    if (log_file == NULL) {
        error("Error al abrir el archivo de registro");
    }

    fprintf(log_file, "[%s] %s\n\n", time_str, message);

    fclose(log_file);
}

// Analiza la línea de solicitud HTTP para determinar el método, la ruta y el host
void parse_request_line(char *buffer, http_request *request) {
    char *method_end = strchr(buffer, ' ');
    if (method_end == NULL) {
        request->method = UNSUPPORTED;
        return;
    }
    size_t method_len = method_end - buffer;
    if (strncmp(buffer, "GET", method_len) == 0) {
        request->method = GET;
    } else if (strncmp(buffer, "POST", method_len) == 0) {
        request->method = POST;
    } else if (strncmp(buffer, "PUT", method_len) == 0) {
        request->method = PUT;
    } else if (strncmp(buffer, "DELETE", method_len) == 0) {
        request->method = DELETE;
    } else {
        request->method = UNSUPPORTED;
    }
    printf("--> Método HTTP: %d\n", request->method);

    char *uri_start = method_end + 1;
    char *uri_end = strchr(uri_start, ' ');
    if (uri_end == NULL) {
        request->method = UNSUPPORTED;
        return;
    }
    size_t uri_len = uri_end - uri_start;
    char *http_version_start = uri_end + 1;
    if (http_version_start[0] != 'H' || http_version_start[1] != 'T' || http_version_start[2] != 'T' || http_version_start[3] != 'P' || http_version_start[4] != '/') {
        request->method = UNSUPPORTED;
        printf("-->http_version_start no comenza con HTTP/. ");
        return;
    }
    if (http_version_start[5] == '0') {
        strcpy(request->version, "HTTP/1.0");
        printf("-->HTTP version 1.0 detectado. ");
    } else if (http_version_start[5] == '1') {
        strcpy(request->version, "HTTP/1.1 ");
        printf("-->HTTP version 1.1 detectado. ");
    } else {
        request->method = UNSUPPORTED;
        printf("--> No se puede detectar la version de HTTP o no esta soportado. ");
        return;
    }
    request->version[7] = '\0';
    char *uri = strndup(uri_start, uri_len);
    char *host_start = strstr(uri, "://");
    if (host_start) {
        host_start += 3;
        char *path_start = strchr(host_start, '/');
        if (path_start) {
            *path_start = '\0';
            path_start++;
            request->host = strdup(host_start);
            request->path = strdup(path_start);
        } else {
            request->host = strdup(host_start);
            request->path = strdup("");
        }
    } else {
        request->host = strdup("");
        request->path = strdup(uri);
    }
    printf("->> Host: %s\n", request->host);
    printf("->> Ruta: %s\n", request->path);
    free(uri);
}



// Función para manejar una conexión de cliente
void handle_connection(int client_fd) {
    char request_buffer[MAX_REQUEST_SIZE];
    ssize_t bytes_received = recv(client_fd, request_buffer, MAX_REQUEST_SIZE - 1, 0);
    if (bytes_received < 0) {
        error("Error al recibir la solicitud HTTP");
    }
    request_buffer[bytes_received] = '\0';
    logger(request_buffer);
    printf("Solicitud HTTP recibida: %s\n", request_buffer);

    // Analizar la línea de solicitud HTTP
    http_request request;
    parse_request_line(request_buffer, &request);
    logger(request.path);

    // Determinar el estado de la solicitud y generar una respuesta HTTP
    char *response_body = NULL;
    size_t response_length = 0;
    int response_code = 0;
    char *status_text = NULL;

    switch (request.method) {
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
         if (response_length_written >= MAX_RESPONSE_SIZE) {
            error("Respuesta HTTP demasiado larga para el búfer");
        }

    // Enviar la respuesta HTTP al cliente
    ssize_t bytes_sent = send(client_fd, response_buffer, strlen(response_buffer), 0);
    if (bytes_sent < 0) {
        error("Error al enviar la respuesta HTTP");
    }
    logger(response_buffer);
    close(client_fd);
}

// Función para el hilo que acepta conexiones de clientes
void *accept_connections(void *server_fd_ptr) {
    int server_fd = *(int *)server_fd_ptr;
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0) {
            error("Error al aceptar la conexión del cliente");
        }
        handle_connection(client_fd);
    }
    return NULL;
}

// Funcion para manejar conexiones de varios clientes
void *connection_handler(void *arg) {
    int client_fd = *(int *)arg;
    handle_connection(client_fd);
    close(client_fd);
    return NULL;
}

int main(int argc, char *argv[]) {
    
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Crea el socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Error al crear el socket \n");
        exit(EXIT_FAILURE);
    }

    // Configura la dirección y el puerto para el socket
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Vincula el socket al puerto y la dirección especificados
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error al vincular el socket \n");
        exit(EXIT_FAILURE);
    }

    // Escucha las conexiones entrantes
    if (listen(server_fd, MAX_CONNECTIONS) < 0) {
        perror("Error al escuchar en el socket \n");
        exit(EXIT_FAILURE);
    }

    printf("Servidor iniciado en el puerto %d...\n", PORT);

    while (1) {
        // Acepta una nueva conexión entrante
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Error al aceptar la conexión entrante \n");
            continue;
        }

        // Crea un nuevo hilo para manejar la conexión entrante
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, connection_handler, (void *)&client_fd) < 0) {
            perror("Error al crear el hilo \n");
            continue;
        }

        // El hilo manejará la conexión, así que podemos continuar aceptando conexiones entrantes
        pthread_detach(thread_id);
    }

    return 0;
}
