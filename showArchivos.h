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
int showFile(int client, char * ruta);

// End of header file
#endif
