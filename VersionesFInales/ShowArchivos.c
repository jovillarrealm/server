#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    FILE *file;
    char *buffer;
    int fileLen;

    // Open file
    file = fopen("html/index.html", "rb");
    if (!file) {
        perror("File error");
        return EXIT_FAILURE;
    }

    // Get file length
    fseek(file, 0, SEEK_END);
    fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory
    buffer = (char *)malloc(fileLen);
    if (!buffer) {
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
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Accept-Ranges: bytes\r\n"
        "Connection: close\r\n"
        "\r\n",
        fileLen);

    // Create socket
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        perror("Socket error");
        return EXIT_FAILURE;
    }


    // Fix de "address already in use"
    int _ = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &_, sizeof(_));



    // Bind socket to port
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8081);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind error");
        return EXIT_FAILURE;
    }

    // Start listening for incoming connections
    if (listen(sd, SOMAXCONN) < 0) {
        perror("Listen error");
        return EXIT_FAILURE;
    }

    for (;;) {
        // Accept incoming connection
        int client = accept(sd, NULL, NULL);
        if (client < 0) {
            perror("Accept error");
            continue;
        }

        // Send HTTP response header
        int sent = send(client, header, strlen(header), 0);
        if (sent != strlen(header)) {
            perror("Send error");
        }

        // Send file data in chunks until the entire file is sent
        int bytes_sent = 0;
        int chunk_size = 1024;
        while (bytes_sent < fileLen) {
            int remaining = fileLen - bytes_sent;
            int send_size = remaining > chunk_size ? chunk_size : remaining;
            int sent = send(client, buffer + bytes_sent, send_size, 0);
            if (sent == -1) {
                perror("Send error");
                break;
            }
            bytes_sent += sent;
        }

    // Receive and discard incoming data until the connection is closed
    while (recv(client, buffer, fileLen, MSG_DONTWAIT) > 0) {}


}

    free(buffer);
    close(sd);

    return EXIT_SUCCESS;
}