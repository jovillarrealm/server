
# **server_side_c**

El punto es hacer un servidor en C con HTTP/1.1
Aquí es donde:

- Daniel
- Sara
- Jorge

vamos a estar haciendo cosas.

******

## **Tabla de contenido**

1. [Inicio](#inicio)
2. [Teoría](#teoría)
3. [Miscelánea](#misc)

******

# **Inicio**

De momento se planea tener en una imagen de ubuntu de AWS, o algo, entonces en vez de lidiar con las mierdas de windows de no ser POSIX compliant con freaking sockets no es algo con lo que quiera lidiar.

## **Setup**

1. Prender un [WSL de ubuntu.](https://learn.microsoft.com/es-mx/windows/wsl/install) y [WSL en vscode.](https://code.visualstudio.com/docs/remote/wsl)
2. Desde wsl se usa con [Meson.](https://mesonbuild.com/SimpleStart.html)
3. Sino, CMakeTools.

### **Detalles**

|Compilador|Build|Debugger|
|---|---|---|
|gcc|meson|gdb|

Debian, Ubuntu and derivatives:

``` bash
sudo apt install build-essential gdb
sudo apt install meson ninja-build
```

Para instalar gcc y gdb

``` bash
sudo apt install build-essential gdb
```

Para instalar meson y ninja

``` bash
sudo apt install meson ninja-build
```

Para iniciar un projecto en el directorio actual

``` bash
meson init --name server --build
```

Para compilar y testeo

``` bash
meson compile -C builddir
meson test -C builddir
```

Instalar

``` bash
DESTDIR=/path/to/staging/root/borrardespues meson install -C builddir
```

******

# **Teoría**

## Sockets

["a way to speak to other programs using standard Unix file descriptors"](https://man7.org/linux/man-pages/man2/socket.2.html)

but also, everything in Unix is a file, entonces esto es una forma de IPC (Interprocess Comunication) sobre una red.

para TCP vamos a necesitar:

``` C
int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
```

Para entender esa línea se requiere contexto.
Porque ```socket()``` retorna el Unix File Handle (```int```).
[AF_INET, SOCK_STREAM](https://man7.org/linux/man-pages/man7/ip.7.html),  tambien necesitan de más contexto.

``` C
int socket(int domain, int type, int protocol);
#include <netinet/in.h>
#include <netinet/ip.h> /*superset of previous*/ 
//domain, type, y protocol

```

```domain : AF_INET``` se refiere IPv4 Internet protocols; ```AF_INET6``` sería para IPv6 Internet protocols.

```type : SOCK_STREAM``` se refiere a que se va a abrir un stream socket, necesario para stream de bytes; ```SOCK_DGRAM``` necesario para manejo de datagramas.

```protocol : 0``` sirve para lo que sea, ```IPPROTO_TCP``` para ```TCP```, ```IPPROTO_UDP``` para ```UDP```.

## bind

algo se supone que facil, asociar socket con puerto.
Y casi lo es: pero hay que justificar un cast extraño:

``` C
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen); //declarando

bind(int tcp_socket, (struct sockaddr *)&server_addr, sizeof(server_addr))// en practica
```

Primero, aquí se usa el File Descriptor que haya devuelto ```socket```.

Segundo, se pide un ```sockaddr```. Basicamente, esto existe porque hay muchas diferencias entre las diferentes familias de protocolos, y sus tamaños, por tantp, para solucionar lidiar con esas diferencias, solo se pide que desde un struct que defina la informacion de un protocolo como ```sockaddr_in``` o ```sockaddr_in6``` simplemente se hace un cast a algo genérico. ```sockaddr``` es ese algo genérico.

Entender la diferencia entre estos tipos además requiere algo de miseria con estandares de POSIX, C y sus compiladores, linux, y más networking.[Rabbit hole](https://stackoverflow.com/questions/18609397/whats-the-difference-between-sockaddr-sockaddr-in-and-sockaddr-in6) [hole](https://stackoverflow.com/questions/48328708/c-create-a-sockaddr-struct) si quiere. Basicamente hay que hacer uso de los structs de diferentes familias, que deben ser casteables a este tipo, para tener algo más o menos general.

para bindear con puertos <1024 hay que correr el programa con sudo, porque solo ```root``` tiene acceso a esos puertos.

## listen

Este fue muy fácil, literal fue pasarle el socket, y el numero máximo de conecciones permitidas. A cada una se le asigna un pthread entonces el paralelismo de la máquina debe estar por ahí.

``` C
listen(tcp_socket, MAX_CONNECTIONS)
```

## accept

``` C
client_socket = accept(tcp_socket_in, (struct sockaddr *)&client_addr_in, (socklen_t*)sizeof(client_addr_in));
```

retorna un fd de un socket al que se le puede hacer send y recv()

## connect()

No se va a usar en este trabajo. alternativa a accept.

## recv()

Si hay alguna vaina de Keep-Alive se puede hacer uso de esta función para mas de un request por conexión

## send

De momento se hace uso de write() que es equivalente salvo el uso de flags.
Si hay una razón de cambio, estaría relacionada con el uso del MSG_MORE, en send y TCP_CORK en socket para enviar varios paquetes en una misma conexión.

# Misc

``` bash
netstat
```

Puertos: DNS 53, SSH 22, HTTP 80.

# FIXME

Vamos a necesitar implementar connection Keep alive, hacer uso de timers if so

# Three way Handshake

C ->syn=1                    S

C <-ack=1; syn=1             S

C ->ack=1                    S

Connection: Keep alive ; que no sean conecciones shortlived, que se puedan hacer varios requests ahi.
