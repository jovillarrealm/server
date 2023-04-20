#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "http_request.h"
#include "deleteArchivos.h"

int deleteFile(int client, char *ruta)
{
    if (strncmp(ruta, "./assets/", 9) != 0) {
        printf("Error deleting file, only files on 'assets' can be deleted %s\n", ruta);
        return -1;
    }
    
    int ret = remove(ruta);
    if (ret != 0) {
        printf("Error deleting file: %s\n", ruta);
        return -1;
    }
    return 0;
}