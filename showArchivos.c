#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "http_request.h"
#include "showArchivos.h"
#include "sendArchivos.h"
#include "sendArchivos.c"

int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int showFile(int client, char *ruta)
{
    FILE *file;
    char *buffer;
    char *http_mime = (char *)malloc(50 * sizeof(char));
    int fileLen = 0;

    // Open file
    file = fopen(ruta, "rb");
    if (!file || !is_regular_file(ruta))
    {
        char header[1024];
        char *now = get_current_time();
        sprintf(header,
                "HTTP/1.1 404 Not Found\r\n"
                "Date: %s\r\n"
                "Server: SaranaiServer/1.0\r\n"
                "Content-Type: text/html\r\n"
                "Connection: close\r\n"
                "\r\n",
                now);
        free(now);

        // Send HTTP response header
        ssize_t sent = send(client, header, strlen(header), 0);
        if (sent != (ssize_t)strlen(header))
        {
            perror("Send error");
        }

        // Open and send 404 error page
        FILE *error_page = fopen("404/404page.html", "rb");
        if (error_page)
        {
            char buffer[4096];
            size_t read_size;
            while ((read_size = fread(buffer, 1, sizeof(buffer), error_page)) > 0)
            {
                ssize_t sent = send(client, buffer, read_size, 0);
                if (sent == -1)
                {
                    perror("Send error");
                    break;
                }
            }
            fclose(error_page);
            free(http_mime);
            return 0;
        }
        else
        {
            // If the 404 error page could not be opened, send a simple error message instead
            const char *error_message = "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1><p>The requested resource could not be found.</p></body></html>";
            ssize_t sent = send(client, error_message, strlen(error_message), 0);
            if (sent == -1)
            {
                perror("Send error");
            }
        }
        free(http_mime);
        return 0;
    }

    get_mime_from_path(ruta, http_mime);

    if (http_mime == "na")
    {
        sendFile(client, ruta);
        fclose(file);
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
    if (sent != (ssize_t)strlen(header))
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

    while (recv(client, buffer, fileLen, MSG_DONTWAIT) > 0)
    {
    }

    free(http_mime);
    free(buffer);
    
    return EXIT_SUCCESS;
}

// requiere un path y un http_mime que ya tenga memoria malloc-eada
int get_mime_from_path(char *ruta, char *http_mime)
{
    char *file_type;
    file_type = strrchr(ruta, '.'); // Checks file extension

    if (strcmp(file_type, ".html") == 0)
    {
        strcpy(http_mime, "text/html");
    }
    else if (strcmp(file_type, ".txt") == 0)
    {
        strcpy(http_mime, "text/plain");
    }
    else if (strcmp(file_type, ".css") == 0)
    {
        strcpy(http_mime, "text/css");
    }
    else if (strcmp(file_type, ".js") == 0)
    {
        strcpy(http_mime, "application/javascript");
    }
    else if (strcmp(file_type, ".json") == 0)
    {
        strcpy(http_mime, "application/json");
    }
    else if (strcmp(file_type, ".xml") == 0)
    {
        strcpy(http_mime, "application/xml");
    }
    else if (strcmp(file_type, ".pdf") == 0)
    {
        strcpy(http_mime, "application/pdf");
    }
    else if (strcmp(file_type, ".jpg") == 0 || strcmp(file_type, ".jpeg") == 0)
    {
        strcpy(http_mime, "image/jpeg");
    }
    else if (strcmp(file_type, ".png") == 0)
    {
        strcpy(http_mime, "image/png");
    }
    else if (strcmp(file_type, ".gif") == 0)
    {
        strcpy(http_mime, "image/gif");
    }
    else if (strcmp(file_type, ".svg") == 0)
    {
        strcpy(http_mime, "image/svg+xml");
    }
    else if (strcmp(file_type, ".ico") == 0)
    {
        strcpy(http_mime, "image/x-icon");
    }
    else if (strcmp(file_type, ".mp3") == 0)
    {
        strcpy(http_mime, "audio/mpeg");
    }
    else if (strcmp(file_type, ".wav") == 0)
    {
        strcpy(http_mime, "audio/wav");
    }
    else if (strcmp(file_type, ".mp4") == 0)
    {
        strcpy(http_mime, "video/mp4");
    }
    else if (strcmp(file_type, ".avi") == 0)
    {
        strcpy(http_mime, "video/x-msvideo");
    }
    else if (strcmp(file_type, ".doc") == 0)
    {
        strcpy(http_mime, "application/msword");
    }
    else if (strcmp(file_type, ".xls") == 0)
    {
        strcpy(http_mime, "application/vnd.ms-excel");
    }
    else if (strcmp(file_type, ".ppt") == 0)
    {
        strcpy(http_mime, "application/vnd.ms-powerpoint");
    }
    else if (strcmp(file_type, ".md") == 0)
    {
        strcpy(http_mime, "text/markdown");
    }
    else
    {
        strcpy(http_mime, "na");
    }
    return 0;
}



