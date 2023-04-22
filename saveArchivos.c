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
    FILE *fp = NULL;

    if (request->content_type != NULL && request->path != NULL && request->body != NULL)
    {
        if (strcmp(request->content_type, "application/form-data") ==0|| strcmp(request->content_type, "application/x-www-form-urlencoded")==0)
        {
            printf("%s\n", request->content_type );
            char path[250];
            if (strncmp(request->content_type, "application/form-data", 21))
            {
                sprintf(path, "%s/post_files/postform_data.txt", request->doc_root_folder);
                fp = fopen(path, "a+");
            }
            else if (strncmp(request->content_type, "application/x-www-form-urlencoded", 33))
            {
                sprintf(path, "%s/post_files/postform_url.txt", request->doc_root_folder);
                fp = fopen(path, "a+");
            }
            if (fp == NULL)
            {
                printf("Error opening file\n");
                return;
            }
            // Escribir el cuerpo del mensaje en el archivo, junto con la fecha y hora actual
            fprintf(fp, "-----------------------\n");
            fprintf(fp, "Timestamp: %s\n", timestamp);
            fprintf(fp, "File contents:\n");
            fprintf(fp, "%s\n\n", (char *)request->body);

            char header[1024];
            char *now = get_current_time();
            sprintf(header,
                    "HTTP/1.1 200 OK\r\n"
                    "Date: %s\r\n"
                    "Server: SaranaiServer/1.0\r\n"
                    "Connection: keep-alive\r\n"
                    "Keep-Alive: timeout=5, max=100\r\n"
                    "\r\n",
                    now);
            free(now);

            // Send HTTP response header
            ssize_t sent = send(client_fd, header, strlen(header), 0);
            if (sent != (ssize_t)strlen(header))
            {
                perror("Send error");
            }
        }
        else
        {
            // Binary

            fp = fopen(request->path, "wb");
            if (fp == NULL)
            {
                printf("Error opening file\n");
                return;
            }

            fwrite(request->body, request->body_size, 1, fp);

            char header[1024];
            char *now = get_current_time();
            sprintf(header,
                    "HTTP/1.1 201 Created\r\n"
                    "Date: %s\r\n"
                    "Server: SaranaiServer/1.0\r\n"
                    "Connection: keep-alive\r\n"
                    "Keep-Alive: timeout=5, max=100\r\n"
                    "\r\n",
                    now);
            free(now);

            // Send HTTP response header
            ssize_t sent = send(client_fd, header, strlen(header), 0);
            if (sent != (ssize_t)strlen(header))
            {
                perror("Send error");
            }
            /*
            // Send file data in chunks until the entire file is sent
            int bytes_sent = 0;
            int chunk_size = 1024;
            while (bytes_sent < request->body_size)
            {
                int remaining = request->content_len - bytes_sent;
                int send_size = remaining > chunk_size ? chunk_size : remaining;
                int sent = send(client_fd, request->body + bytes_sent, send_size, 0);
                if (sent == -1)
                {
                    perror("Send error");
                    break;
                }
                bytes_sent += sent;
            }
            */
        }
        // Cerrar el archivo
        fclose(fp);
    }
}