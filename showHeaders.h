#ifndef SHOWHEADERS_H
#define SHOWHEADERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "http_request.h"
#include "showHeaders.h"

// Functions

// GET

void showHeaders(int client, char *ruta);

// End of header file
#endif