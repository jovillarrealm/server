#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
    char *body;
    char version[16];
    int status_code;
    int content_len;
    char *content_type;
} http_request;

// Define la estructura para pasar a los hilos de cada conexión
typedef struct connection_info
{
    int client_fd;
    FILE *log_file;
} connection_info;

void prequest(http_request *req);

// Aux para saber el tiempo exactamente
char *get_current_time(void);

// Muestra un mensaje de error y sale del programa
void error(const char *msg);
#endif