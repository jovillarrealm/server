#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

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
} http_request;

#endif