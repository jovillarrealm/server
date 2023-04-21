#ifndef SHOWARCHIVOS_H
#define SHOWARCHIVOS_H

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


// Functions

//GET
int get_mime_from_path(char *ruta, char *http_mime);

int showFile(int client, char * ruta);

// End of header file
#endif
