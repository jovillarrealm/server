#ifndef SENDARCHIVOS_H
#define SENDARCHIVOS_H

// #includes

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

// Functions

int sendFile(int client, char * ruta);

// End of header file
#endif
