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