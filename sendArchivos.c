#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "sendArchivos.h"

int sendFile(int client)
{
    FILE *file;
    char *buffer;
    long fileLen;

    // Open file
    file = fopen("opm.mp4", "rb");
    if (!file)
    {
        perror("File error");
        return EXIT_FAILURE;
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
        fclose(file);
        return EXIT_FAILURE;
    }

    // Read file contents into buffer
    fread(buffer, fileLen, 1, file);
    fclose(file);

    char header[1024];
    sprintf(header,
            "HTTP/1.1 200 OK\r\n"
            "Date: Thu, 19 Feb 2023 12:27:04 GMT\r\n"
            "Server: SaranaiServer/2.2.3\r\n"
            "Last-Modified: Wed, 18 Jun 2020 16:05:58 GMT\r\n"
            "ETag: \"56d-9989200-1132c580\"\r\n"
            "Content-Type: application/octet-stream\n"
            "Content-Disposition: attachment; filename=\"opm.mp4\"\r\n"
            "Content-Length: %ld\r\n"
            "Accept-Ranges: bytes\r\n"
            "Connection: keep-alive\r\n"
            "Keep-Alive: timeout=5, max=100\r\n"
            "\r\n",
            fileLen);

    // Send HTTP response header
    int sent = send(client, header, strlen(header), 0);
    if ((size_t)sent != strlen(header))
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

    // Receive and discard incoming data until the connection is closed
    while (recv(client, buffer, fileLen, MSG_DONTWAIT) > 0)
    {
    }

    free(buffer);

    return EXIT_SUCCESS;
}