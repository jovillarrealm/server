#ifndef DELETEARCHIVOS_H
#define DELETEARCHIVOS_H

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


// Functions

//GET
int deleteFile(int client, char * ruta);

// End of header file
#endif
