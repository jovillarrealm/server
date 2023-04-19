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
    char body_binary[100000]; 
    char *file_ext;
    char version[16];
    int status_code;
    int content_len;
    char *content_type;
} http_request;

void prequest(http_request req)
{

    printf("--Method: %d\n", req.method);
    if (req.host != NULL)
        printf("--Host: %s\n", req.host);
    if (req.path!=NULL)
        printf("--Path: %s\n", req.path);
    if (req.body!=NULL)
        printf("--Body: %s\n", req.body);
    //if (req.status_code)    
    if (req.content_len!=0)
        printf("--Content-Length: %d\n", req.content_len);
    if (req.content_type)
        printf("--Content-Type: %s\n", req.content_type);
}
#endif