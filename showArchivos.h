#ifndef SHOWARCHIVOS_H
#define SHOWARCHIVOS_H

// #includes

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include "showArchivos.h"

// Functions
char *get_time(void);

int showFile(int client, char *ruta);

// End of header file
#endif
