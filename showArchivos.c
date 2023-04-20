#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "http_request.h"
#include "showArchivos.h"
#include "sendArchivos.h"
#include "sendArchivos.c"



int showFile(int client, char *ruta)
{
    FILE *file;
    char *buffer;
    char *http_mime = (char *)malloc(50 * sizeof(char));
    int fileLen=0;


    

    // Open file
    file = fopen(ruta, "rb");
    if (!file)
    {
        perror("File could not be opened");
        free(http_mime);

        return 0;
    }


    get_mime_from_path(ruta, http_mime);

    if (http_mime == "na"){
        sendFile(client, ruta);
        return 0;
    }




    // Get file length
    fseek(file, 0, SEEK_END);
    fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory
    buffer = (char *)malloc(fileLen);
    if (!buffer)
    {
        perror("Memory error");
        free(http_mime);
        fclose(file);
        return 0;
    }

    // Read file contents into buffer
    fread(buffer, fileLen, 1, file);
    fclose(file);

    char header[1024];
    char *now = get_current_time();
    sprintf(header,
        "HTTP/1.1 200 OK\r\n"
        "Date: %s\r\n"
        "Server: SaranaiServer/1.0\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: keep-alive\r\n"
        "Keep-Alive: timeout=5, max=100\r\n"
        "\r\n",
        now, http_mime, fileLen);
        free(now);


    // Send HTTP response header
    ssize_t sent = send(client, header, strlen(header), 0);
    if (sent != (ssize_t) strlen(header))
    {
        perror("Send error");
    }

    // Send file data in chunks until the entire file is sent
    int bytes_sent = 0;
    int chunk_size = 1024;
    while (bytes_sent < fileLen)
    {
        int remaining = fileLen - bytes_sent;
        int send_size = remaining > chunk_size ? chunk_size : remaining;
        int sent = send(client, buffer + bytes_sent, send_size, 0);
        if (sent == -1)
        {
            perror("Send error");
            break;
        }
        bytes_sent += sent;
    }

        while (recv(client, buffer, fileLen, MSG_DONTWAIT) > 0) {}

    free(http_mime);
    free(buffer);
    close(client);
    return EXIT_SUCCESS;
}