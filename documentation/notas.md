
# **Notas de ./server**

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

## **Inicio**

De momento de escalabilidad tenemos un accept metido en un while con detached threads. epoll() probablemente va tocar.

### **Setup**

1. Prender un [WSL de ubuntu.](https://learn.microsoft.com/es-mx/windows/wsl/install) y [WSL en vscode.](https://code.visualstudio.com/docs/remote/wsl)
2. Desde wsl se usa con [Meson.](https://mesonbuild.com/SimpleStart.html)

#### **Detalles**

No hay profiler 😒.
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

Instalar algo (probablemente no se va a usar)

``` bash
DESTDIR=/path/to/staging/root/borrardespues meson install -C builddir
```

Mover los json con la extension de mesonbuild para vscode es ❤️‍🔥❤️‍🔥❤️‍🔥❤️‍🔥, corre cada vez que se abre el workspace para preguntar si quieres configurar (incluso si ya está).

Lo bueno es que moviendo los task.json eso queda bonito.

******

## **Teoría**

### Sockets

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

#### bind

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

#### listen

Este fue muy fácil, literal fue pasarle el socket, y el numero máximo de conecciones permitidas. A cada una se le asigna un pthread entonces el paralelismo de la máquina debe estar por ahí.

``` C
listen(tcp_socket, MAX_CONNECTIONS)
```

#### accept

``` C
client_socket = accept(tcp_socket_in, (struct sockaddr *)&client_addr_in, (socklen_t*)sizeof(client_addr_in));
```

retorna un fd de un socket al que se le puede hacer send y recv()

#### connect()

No se va a usar en este trabajo. alternativa a accept.

#### recv()

Si hay alguna vaina de Keep-Alive se puede hacer uso de esta función para mas de un request por conexión

#### send

De momento se hace uso de write() que es equivalente salvo el uso de flags.
Si hay una razón de cambio, estaría relacionada con el uso del MSG_MORE, en send y TCP_CORK en socket para enviar varios paquetes en una misma conexión.

## Misc

``` bash
netstat
```

Puertos: DNS 53, SSH 22, HTTP 80.

``` C
# define _POSIX_C_SOURCE 200809
# include <netdb.h>

// Para imprimir y retornar la lista de IPs asociadas a la dirección provista.
// Esta función va a permitir imprimir lista de direcciones, y usará la primera en el servidor
// Esto para posible debugging futuro
// De: <https://beej.us/guide/bgnet/html/split/system-calls-or-bust.html>
struct addrinfo *get_addrinfo_linked_list(char*some_IP, char *port)
{
    printf("De host: %s\n", some_IP);
    struct addrinfo addr_hints = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM};
struct addrinfo*addrinfo_list;
    if (getaddrinfo(some_IP, port, &addr_hints, &addrinfo_list) != 0)
        error("getaddrinfo error");
    char ipstr[INET6_ADDRSTRLEN];
    for (struct addrinfo *p = addrinfo_list; p != NULL; p = p->ai_next)
    {
void*addr;
        char *ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET)
        { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        }
        else
        { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %s: %s\n", ipver, ipstr);
    }

    return addrinfo_list;
}

    size_t host_name_len = 30; // arbitrario
    char host_name[host_name_len];
    if (gethostname(host_name, host_name_len) == -1)
        error("gethostname error");


    struct addrinfo *possible_list = get_addrinfo_linked_list(host_name, argv[1]);


    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size=sizeof(their_addr);
```
