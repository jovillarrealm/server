#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "http_request.h"

void saveFile(http_request *request)
{
    // Abrir el archivo en modo de escritura
        FILE *fp = fopen("./post_files/postLog.txt", "w");
        if (fp == NULL) {
            printf("Error opening file\n");
            return;
        }

        // Escribir el cuerpo del mensaje en el archivo
        fprintf(fp, "%s", request->body);

        // Cerrar el archivo
        fclose(fp);
}