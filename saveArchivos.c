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
    FILE *fp;

    if (request->content_type != NULL && request->path != NULL)
    {
        // POST FILE

        fp = fopen(request->path, "wb");
        if (fp == NULL)
        {
            printf("Error opening file\n");
            return;
        }

        if (request->body_size + request->header_size == MAX_REQUEST_SIZE)
        {
            //EVIL?? realloc habla de aliasing, body size crece en montos discretos indepedientemente de los datos
            
            request->body=realloc(request->body, request->body_size + MAX_REQUEST_SIZE);
            request->body_size += MAX_REQUEST_SIZE; 

            size_t new_chunk = read(client_fd, request->body, MAX_REQUEST_SIZE);
        }
        fwrite(request->body, request->body_size, 1, fp);

        // fwrite(re)
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
        // fprintf(fp, "%s\n\n", request->body);
    }

    // Cerrar el archivo
    fclose(fp);
}