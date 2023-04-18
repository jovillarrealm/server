#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#include "http_request.h"

void saveFile(http_request *request)
{
    printf("Saving file as: ...\n");
    
    // Obtener la fecha y hora actual
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%c", tm);

    // Abrir el archivo en modo de adición
    FILE *fp = fopen("./post_files/postLog.txt", "a");
    if (fp == NULL) {
        printf("Error opening file\n");
        return;
    }

    // Escribir el cuerpo del mensaje en el archivo, junto con la fecha y hora actual
    fprintf(fp, "-----------------------\n");
    fprintf(fp, "Timestamp: %s\n", timestamp);
    fprintf(fp, "File contents:\n");

    // Verificar si el cuerpo del mensaje no está vacío
    if (request->body != NULL && strlen(request->body) > 0)
    {
        // Escribir el contenido del cuerpo del mensaje en el archivo
        fprintf(fp, "%s\n\n", request->body);
    }
    // Verificar si el cuerpo del mensaje está vacío y el cuerpo binario no lo está
    else if (request->binary_body != NULL)
    {
        // Verificar si el tipo de contenido es uno de los tipos permitidos
        if (strstr(request->content_type, "text/plain") == NULL &&
            (strstr(request->content_type, "image/png") != NULL ||
             strstr(request->content_type, "image/jpeg") != NULL ||
             strstr(request->content_type, "image/gif") != NULL ||
             strstr(request->content_type, "application/pdf") != NULL ||
             strstr(request->content_type, "application/octet-stream") != NULL ||
             strstr(request->content_type, "video/mp4") != NULL ||
             strstr(request->content_type, "audio/mp3") != NULL))
        {
            // Crear el archivo con el mismo nombre que el archivo enviado en el POST
            char *filename = strrchr(request->path, '/');
            if (filename == NULL) {
                printf("Error getting filename\n");
                fclose(fp);
                return;
            }
            filename++;

            FILE *file = fopen(filename, "wb");
            if (file == NULL) {
                printf("Error opening file\n");
                fclose(fp);
                return;
            }

            // Guardar el contenido del archivo en el nuevo archivo creado
            fwrite(request->binary_body, request->content_len, 1, file);

            // Cerrar el archivo
            fclose(file);

            fprintf(fp, "File saved: %s\n\n", filename);
        }
        else
        {
            // Si el tipo de contenido es "text/plain", guardar solo el cuerpo del mensaje
            fprintf(fp, "%s\n\n", (char *)request->binary_body);
        }
    }

    // Cerrar el archivo
    fclose(fp);
}
