
#include "showArchivos.h"



char *get_time(void){
    time_t t = time(NULL);
    struct tm tm = *gmtime(&t);
    char *time_str = malloc(30 * sizeof(char));
    strftime(time_str, 30, "%a, %d %b %Y %H:%M:%S GMT", &tm);
    return time_str;
}

int showFile(int client, char *ruta) {
    FILE *file;
    char *buffer;
    char *file_type;
    char *http_mime = (char *)malloc(50 * sizeof(char));
    int fileLen;


    

    // Open file
    file = fopen(ruta, "rb");
    if (!file) {
        perror("File error");
        free(http_mime);
        return EXIT_FAILURE;
    }


    file_type = strrchr(ruta, '.'); //Checks file extension

     if(strcmp(file_type, ".html") == 0) {
        strcpy(http_mime, "text/html");
    } else if(strcmp(file_type, ".txt") == 0) {
        strcpy(http_mime, "text/plain");
    } else if(strcmp(file_type, ".css") == 0) {
        strcpy(http_mime, "text/css");
    } else if(strcmp(file_type, ".js") == 0) {
        strcpy(http_mime, "application/javascript");
    } else if(strcmp(file_type, ".json") == 0) {
        strcpy(http_mime, "application/json");
    } else if(strcmp(file_type, ".xml") == 0) {
        strcpy(http_mime, "application/xml");
    } else if(strcmp(file_type, ".pdf") == 0) {
        strcpy(http_mime, "application/pdf");
    } else if(strcmp(file_type, ".jpg") == 0 || strcmp(file_type, "jpeg") == 0) {
        strcpy(http_mime, "image/jpeg");
    } else if(strcmp(file_type, ".png") == 0) {
        strcpy(http_mime, "image/png");
    } else if(strcmp(file_type, ".gif") == 0) {
        strcpy(http_mime, "image/gif");
    } else if(strcmp(file_type, ".svg") == 0) {
        strcpy(http_mime, "image/svg+xml");
    } else if(strcmp(file_type, ".ico") == 0) {
        strcpy(http_mime, "image/x-icon");
    } else if(strcmp(file_type, ".mp3") == 0) {
        strcpy(http_mime, "audio/mpeg");
    } else if(strcmp(file_type, ".wav") == 0) {
        strcpy(http_mime, "audio/wav");
    } else if(strcmp(file_type, ".mp4") == 0) {
        strcpy(http_mime, "video/mp4");
    } else if(strcmp(file_type, ".avi") == 0) {
        strcpy(http_mime, "video/x-msvideo");
    } else if(strcmp(file_type, ".doc") == 0) {
        strcpy(http_mime, "application/msword");
    } else if(strcmp(file_type, ".xls") == 0) {
        strcpy(http_mime, "application/vnd.ms-excel");
    } else if(strcmp(file_type, ".ppt") == 0) {
        strcpy(http_mime, "application/vnd.ms-powerpoint");
    } else {
        printf("Unsupported file type!\n");
        free(http_mime);
        fclose(file);
        return 1;
    }





    // Get file length
    fseek(file, 0, SEEK_END);
    fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory
    buffer = (char *)malloc(fileLen);
    if (!buffer) {
        perror("Memory error");
        free(http_mime);
        fclose(file);
        return EXIT_FAILURE;
    }

    // Read file contents into buffer
    fread(buffer, fileLen, 1, file);
    fclose(file);


    char header[1024];
    char *now = get_time();
    sprintf(header,
            "HTTP/1.1 200 OK\r\n"
            "Date: %s\r\n"
            "Server: SaranaiServer/1.0\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %d\r\n"
            "Accept-Ranges: bytes\r\n"
            "Connection: keep-alive\r\n"
            "Keep-Alive: timeout=5, max=100\r\n"
            "\r\n",
            now, http_mime, fileLen);
        free(now);

    // Send HTTP response header
    int sent = send(client, header, strlen(header), 0);
    if (sent != (int) strlen(header))
    {
        perror("Send error");
        }

        // Send file data in chunks until the entire file is sent
        int bytes_sent = 0;
        int chunk_size = 1024;
        while (bytes_sent < fileLen) {
            int remaining = fileLen - bytes_sent;
            int send_size = remaining > chunk_size ? chunk_size : remaining;
            sent = send(client, buffer + bytes_sent, send_size, 0);
            if (sent == -1) {
                perror("Send error");
                break;
            }
            bytes_sent += sent;
        }

    // Receive and discard incoming data until the connection is closed
    while (recv(client, buffer, fileLen, MSG_DONTWAIT) > 0) {}




    free(buffer);
    close(client);
    return 0;
}
