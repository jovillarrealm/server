#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#include "saveArchivos.h"
#include "http_request.h"

void saveFile(http_request *request, int client_fd)
{
    // Obtener la fecha y hora actual
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%c", tm);
    FILE * fp;

    
    if (request->content_type!=NULL)
    {
        fp = fopen(request->path, "wb");
        if (fp == NULL)
        {
            printf("Error opening file\n");
            return;
        }
        char buf[request->content_len];
        read(client_fd,buf,request->content_len);
    }
    else
    {
        // Abrir el archivo en modo de adición
        fp = fopen("./post_files/postLog.txt", "a");
        if (fp == NULL)
        {
            printf("Error opening file\n");
            return;
        }
        // Escribir el cuerpo del mensaje en el archivo, junto con la fecha y hora actual
        fprintf(fp, "-----------------------\n");
        fprintf(fp, "Timestamp: %s\n", timestamp);
        fprintf(fp, "File contents:\n");
        fprintf(fp, "%s\n\n", request->body);
    }

    // Cerrar el archivo
    fclose(fp);
}