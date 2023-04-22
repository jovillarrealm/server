#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 15
#define MAX_REQUEST_SIZE 65535
#define MAX_RESPONSE_SIZE 65535
#define OK 200
#define BAD_REQUEST 400
#define NOT_FOUND 404



typedef struct http_request
{
    enum method
    {
        GET,
        POST,
        PUT,
        DELETE,
        HEAD,
        UNSUPPORTED
    } method;
    char *host;
    char *path;
    void *body;
    size_t body_size;
    size_t header_size;
    char version[16];
    int status_code;
    size_t content_len;
    char *content_type;
    char *doc_root_folder;
} http_request;

// Define la estructura para pasar a los hilos de cada conexión

//Loggea en el archivo especificado
void logger(const char *message, FILE *log_file);

void prequest(http_request *req);

// Aux para saber el tiempo exactamente
char *get_current_time(void);

// Muestra un mensaje de error y sale del programa
void error(const char *msg);

char *pretty_method(int method);

int get_mime_from_path(char *ruta, char *http_mime);

int listening_socket(u_int16_t port, struct sockaddr_in address);


#endif