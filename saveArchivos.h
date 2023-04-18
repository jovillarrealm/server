#ifndef SAVEARCHIVOS_H
#define SAVEARCHIVOS_H

// #includes

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http_request.h"

// Functions

void saveFile(http_request *request, int client_fd); 

// End of header file
#endif