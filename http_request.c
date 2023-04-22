#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "http_request.h"


void prequest(http_request *req)
{
    if (req->method)
        printf("Method: %s\n", pretty_method(req->method));
    if (req->host != NULL)
        printf("Host: %s\n", req->host);
    if (req->path != NULL)
        printf("Path: %s\n", req->path);
    if (req->body != NULL)
        printf("Body: yes\n");
    if (req->status_code != 200)
        printf("Status Code: %d\n", req->status_code);
    if (req->content_len != 0)
        printf("Content Length: %ld\n", req->content_len);
    if (req->content_type != NULL)
        printf("Content Type: %s\n", req->content_type);
}

// Aux para saber el tiempo exactamente
char *get_current_time(void)
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



char *pretty_method(int method)
{
    if (method == GET)
        return "GET";
    else if (method == POST)
        return "POST";
    else if (method == PUT)
        return "PUT";
    else if (method == DELETE)
        return "DELETE";
    else if (method == HEAD)
        return "HEAD";
    else
        return "UNSUPPORTED";
}

// requiere un path y un http_mime que ya tenga memoria malloc-eada
int get_mime_from_path(char *ruta, char *http_mime)
{
    char *file_type;
    file_type = strrchr(ruta, '.'); // Checks file extension

    if (strcmp(file_type, ".html") == 0)
    {
        strcpy(http_mime, "text/html");
    }
    else if (strcmp(file_type, ".txt") == 0)
    {
        strcpy(http_mime, "text/plain");
    }
    else if (strcmp(file_type, ".css") == 0)
    {
        strcpy(http_mime, "text/css");
    }
    else if (strcmp(file_type, ".js") == 0)
    {
        strcpy(http_mime, "application/javascript");
    }
    else if (strcmp(file_type, ".json") == 0)
    {
        strcpy(http_mime, "application/json");
    }
    else if (strcmp(file_type, ".xml") == 0)
    {
        strcpy(http_mime, "application/xml");
    }
    else if (strcmp(file_type, ".pdf") == 0)
    {
        strcpy(http_mime, "application/pdf");
    }
    else if (strcmp(file_type, ".jpg") == 0 || strcmp(file_type, ".jpeg") == 0)
    {
        strcpy(http_mime, "image/jpeg");
    }
    else if (strcmp(file_type, ".png") == 0)
    {
        strcpy(http_mime, "image/png");
    }
    else if (strcmp(file_type, ".gif") == 0)
    {
        strcpy(http_mime, "image/gif");
    }
    else if (strcmp(file_type, ".svg") == 0)
    {
        strcpy(http_mime, "image/svg+xml");
    }
    else if (strcmp(file_type, ".ico") == 0)
    {
        strcpy(http_mime, "image/x-icon");
    }
    else if (strcmp(file_type, ".mp3") == 0)
    {
        strcpy(http_mime, "audio/mpeg");
    }
    else if (strcmp(file_type, ".wav") == 0)
    {
        strcpy(http_mime, "audio/wav");
    }
    else if (strcmp(file_type, ".mp4") == 0)
    {
        strcpy(http_mime, "video/mp4");
    }
    else if (strcmp(file_type, ".avi") == 0)
    {
        strcpy(http_mime, "video/x-msvideo");
    }
    else if (strcmp(file_type, ".doc") == 0)
    {
        strcpy(http_mime, "application/msword");
    }
    else if (strcmp(file_type, ".xls") == 0)
    {
        strcpy(http_mime, "application/vnd.ms-excel");
    }
    else if (strcmp(file_type, ".ppt") == 0)
    {
        strcpy(http_mime, "application/vnd.ms-powerpoint");
    }
    else if (strcmp(file_type, ".md") == 0)
    {
        strcpy(http_mime, "text/markdown");
    }
    else
    {
        strcpy(http_mime, "na");
    }
    return 0;
}

int listening_socket(u_int16_t port, struct sockaddr_in address)
{
    // Crea el socket
    int server_fd;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Error al crear el socket \n");
        exit(EXIT_FAILURE);
    }

   

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
void parse_lines(char *buffer, http_request *request, char *doc_root_folder)
{
    // Innit request

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
    else if (strncmp(buffer, "HEAD", method_len) == 0)
    {
        request->method = HEAD;
    }
    else if (strncmp(buffer, "DELETE", method_len) == 0)
    {
        request->method = DELETE;
    }
    else
    {
        request->method = UNSUPPORTED;
    }
    printf("--> Método HTTP: %s\n", pretty_method(request->method));

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
        sprintf(path, "%s%s", doc_root_folder, uri);

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
        else
        {
            request->method = UNSUPPORTED;
        }

        char *content_length_start = strstr(method_end, "Content-Length: ");

        if (content_length_start != NULL)
        {
            content_length_start += 16;
            char *content_length_end = strstr(content_length_start, "\r\n");
            size_t content_length_len = content_length_end - content_length_start;
            request->content_len = atoi(strndup(content_length_start, content_length_len));
        }
        else
        {
            request->method = UNSUPPORTED;
        }
    }

    // If we reached here, there were no early returns
    request->status_code = 200;
}

// Parser de HTTP/1.1 requests, y obtiene body si lo necesita
char *parse_request(void *req__buff, ssize_t buff_size, http_request *request, char *doc_root_folder, int client_fd)
{
    char *buffer = (char *)req__buff;
    char *body_start = strstr(buffer, "\r\n\r\n") + 4;
    size_t status_headers_size = body_start - buffer;
    request->header_size = status_headers_size;
    char *status_headers = strndup(buffer, status_headers_size);

    parse_lines(status_headers, request, doc_root_folder);
    if (request->method == UNSUPPORTED)
        request->status_code = 400;
    //  PUT si algo tambien va a tener body

    if ((((ssize_t)status_headers_size < buff_size) || request->method == POST || request->method == PUT) && (request->status_code < 400))
    {
        size_t peabody_size;
        if (request->content_len > MAX_REQUEST_SIZE)
            peabody_size = request->content_len + MAX_REQUEST_SIZE;
        else
            peabody_size = MAX_REQUEST_SIZE;
        void *peabody = malloc(peabody_size);

        size_t body_size = buff_size - status_headers_size;
        memcpy(peabody, req__buff + status_headers_size, body_size);

        while (body_size < request->content_len)
        {
            size_t new_bytes = read(client_fd, peabody + body_size, MAX_REQUEST_SIZE);
            body_size += new_bytes;
        }
        request->body_size = body_size;
        request->body = peabody;
    }
    return status_headers;
}
