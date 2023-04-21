#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "http_request.h"


void prequest(http_request *req)
{
    if (req->method)
        printf("Method: %u\n", req->method);
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