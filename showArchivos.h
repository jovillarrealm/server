#ifndef SHOWARCHIVOS_H
#define SHOWARCHIVOS_H

// #includes

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <bits/types.h>
#include "showArchivos.c"
// Functions

int showFile(int client, char * ruta);

// End of header file
#endif
