#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_MIME_TYPES 40

char *get_mime_type(char *filename);

struct mime_type {
    char *extension;
    char *type;
};

struct mime_type mime_types[MAX_MIME_TYPES] = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".txt", "text/plain"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".xml", "application/xml"},
    {".pdf", "application/pdf"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".bmp", "image/bmp"},
    {".ico", "image/x-icon"},
    {".svg", "image/svg+xml"},
    {".wav", "audio/wav"},
    {".mp3", "audio/mpeg"},
    {".ogg", "audio/ogg"},
    {".mp4", "video/mp4"},
    {".avi", "video/x-msvideo"},
    {".mpeg", "video/mpeg"},
    {".mov", "video/quicktime"},
    {".zip", "application/zip"},
    {".tar", "application/x-tar"},
    {".gz", "application/gzip"},
    {".rar", "application/x-rar-compressed"},
    {".exe", "application/x-msdownload"},
    {".msi", "application/x-msi"},
    {".doc", "application/msword"},
    {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".ppt", "application/vnd.ms-powerpoint"},
    {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {".xls", "application/vnd.ms-excel"},
    {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".odt", "application/vnd.oasis.opendocument.text"},
    {".ods", "application/vnd.oasis.opendocument.spreadsheet"},
    {".odp", "application/vnd.oasis.opendocument.presentation"},
    {".csv", "text/csv"},
    {".mpg", "video/mpeg"},
    {".mpe", "video/mpeg"},
    {".mpeg2", "video/mpeg2"},
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    
    char *filename = argv[1];
    char *mime_type = get_mime_type(filename);
    
    if (mime_type == NULL) {
        printf("Unknown MIME type for file %s\n", filename);
        return 1;
    }
    
    printf("HTTP/1.1 200 OK\r\n");
    printf("Content-Type: %s\r\n\r\n", mime_type);
    
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Error opening file %s\n", filename);
        return 1;
    }
    
    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        fwrite(buffer, 
