#include "http_request.h"
#include <stdio.h>
#include <time.h>
void prequest(http_request *req)
{
    if (req->method)
        printf("Method: %u\n", req->method);
    if (req->host != NULL)
        printf("Host: %s\n", req->host);
    if (req->path != NULL)
        printf("Path: %s\n", req->path);
    if (req->body != NULL)
        printf("Body: %s\n", req->body);
    if (req->status_code != 200)
        printf("Status Code: %d\n", req->status_code);
    if (req->content_len != 0)
        printf("Content Length: %d\n", req->content_len);
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