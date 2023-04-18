#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "http_request.h"
#include "showArchivos.h"
#include "saveArchivos.h"


// git clone --branch testPost3 --single-branch https://github.com/jovillarrealm/server.git



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
void parse_request_line(char *buffer, http_request *request, char* doc_root_folder)
{
    //Innit request

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
        size_t pathlen = strlen(doc_root_folder) + uri_len;
        
        char path[pathlen];
        sprintf(path, "%s%s",doc_root_folder,uri );

        
        request->path = strdup(path);
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
        }

        char *content_length_start = strstr(method_end, "Content-Length: ");

        if (content_length_start != NULL)
        {
            content_length_start += 17;
            char *content_length_end = strstr(content_length_start, "\r\n");
            size_t content_length_len = content_length_end - content_length_start;
            request->content_len = atoi(strndup(content_length_start, content_length_len));
        }

        char *body_start = strstr(buffer, "\r\n\r\n");
        if (body_start != NULL)
        {
            body_start += 4; // saltar los caracteres de separación
            size_t body_len = strlen(body_start);
            request->body = (char *)malloc(body_len + 1);
            memcpy(request->body, body_start, body_len);
            request->body[body_len] = '\0';
            printf("->> Body: %s\n", request->body);
        }
    } else{
    if (request->body!=NULL)
        request->body ="";
    if (request->content_type!=NULL)
        request->content_type="";
    }
    

    
}

// Función para manejar una conexión de cliente
void handle_connection(int client_fd, FILE *log_file, char* doc_root)
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
    http_request request = {.method=0, .body="", .content_len=0,.content_type="",.host="",.path="",.status_code=200,.version=""};
    parse_request_line(request_buffer, &request, doc_root);
    prequest(&request);
    logger(request.path, log_file);

    // Determinar el estado de la solicitud y generar una respuesta HTTP

    switch (request.method)
    {
    case GET:
        printf("lets do a get! \n");
        showFile(client_fd, request.path);
        break;
    case POST:
        printf("lets do a post! \n");
        saveFile(&request, client_fd);
        break;
    default:
        printf("Oh no, bad request! \n");
        break;
    }
}

// Función para el hilo que acepta conexiones de clientes
void *accept_connections(void *server_fd_ptr, char*doc_root_folder)
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
        handle_connection(client_fd, log_file, doc_root_folder);
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
        doc_root_folder = "assets";
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
    printf("Servidor iniciado en el puerto %d...\n", port);

        while (1)
    {
        // Acepta una nueva conexión entrante
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Error al aceptar la conexión entrante \n");
            continue;
        }
        handle_connection(client_fd, log_file, doc_root_folder);
    }
    fclose(log_file); // Unreachable en este momento
    return 0;
}