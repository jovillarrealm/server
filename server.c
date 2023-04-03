#include <stdio.h>
#include <stdlib.h>//exit
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>//socket
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>


/*
Este código crea un socket, lo enlaza a un puerto específico, escucha conexiones entrantes y maneja cada conexión en un bucle infinito.
Dentro del bucle, el código lee los datos enviados por el cliente, imprime la solicitud en la consola, procesa la solicitud (por hacer)
y envía una respuesta de prueba al cliente.
Es importante tener en cuenta que este código solo maneja una conexión a la vez.
Para manejar múltiples conexiones, se debe utilizar una técnica de multiproceso o multihilo. Además, este código no maneja errores
ni verifica la entrada del cliente, lo que lo hace inseguro.
*/

//#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 10
#define MAX_REQUEST_SIZE 4096
#define MAX_RESPONSE_SIZE 4096
#define OK 200
#define BAD_REQUEST 400
#define NOT_FOUND 404
#define CONNECTION_QUEUE_SIZE 5

int str_to_uint16(char *str, uint16_t *res);
void error(const char *msg);
typedef struct http_request http_request;
typedef struct thread_args thread_args;
typedef struct http_req_header http_req_header;
int handle_HEAD(http_request *request, char *response);
int handle_GET(http_request *request, char *response);
int handle_POST(http_request *request, char *response);
int initialize_listening_socket(int http_port);
int parse_request(char *request_string, http_request *request);

// FIXME TODO: Definir la estructura de la petición HTTP:
typedef struct http_request
{
    char method[8];
    char path[256];
    char version[16];
} http_request;

int main(int argc, char *argv[])
{
    //Mock de inicialización

    u_int16_t http_port = 8080;

    FILE *log_file = fopen("logs.txt", "a+");

    char *doc_root_folder = " .";

    if (argc == 4)
    {
        if(str_to_uint16(argv[1], &http_port)==-1)
            error("Error interpretando número de puerto\n");
        log_file = fopen(argv[2], "a+"); // a+ (create + append) option will allow appending which is useful in a log file
        doc_root_folder = argv[3];
    }

    // crea el socket
    int sockfd = initialize_listening_socket(http_port);
     
    // crea el buffer || Creo que esto cabe en un hilo
    /* El buffer es un array de caracteres que se utiliza para almacenar los datos que se leen del socket y que se envían como respuesta al cliente. */
    char buffer[BUFFER_SIZE] = {0};

    // 
    char request_string[MAX_REQUEST_SIZE] = {0};

    /* "cli_addr" es una estructura de tipo sockaddr_in que representa la dirección del socket en el lado del cliente. Cuando se establece una conexión con un cliente, se utiliza esta estructura para almacenar la dirección del cliente. */
    struct sockaddr_in  cli_addr;

    /*La variable clilen es del tipo socklen_t y es utilizada para almacenar el tamaño de la estructura cli_addr, que se utilizará en la función accept() más adelante para aceptar nuevas conexiones. En esta línea se le asigna el valor del tamaño de cli_addr, que es obtenido a través de la función sizeof(). */
    socklen_t clilen = sizeof(cli_addr);

    printf("Server started on port %d...\n", http_port);

    //fd de las conexiónes de clientes
    int newsockfd;

    // cantidad de bytes leidos desde newsockfd y almacenados en el búfer buffer
    int socket_bytes;


    int status_code;
    http_request request;

    // loop de escucha, no sé que tan multihilo pueda ser esto en realidad, later
    while (1)
    {
        /* La función accept() espera y acepta conexiones entrantes.
        Si la conexión se establece correctamente, accept() devuelve un nuevo descriptor de archivo (newsockfd) que se utiliza para enviar y recibir datos en la conexión. */
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd <= 0)
            error("Error on accept\n");

        printf("Conexión aceptada");
        /*  inicializa todos los elementos del buffer buffer con valor 0 */
        bzero(buffer, BUFFER_SIZE);

        /* read() se utiliza para leer los datos enviados por el cliente desde newsockfd y almacenarlos en el búfer buffer. */
        // FIXME HAY QUE MANEJAR PARTIAL SENDS : 7.4 Beej's
        socket_bytes = read(newsockfd, buffer, BUFFER_SIZE);
        if (socket_bytes < 0){
            error("Error reading from socket\n");
        }
        else
        {
            // null terminar el request string por sanidad
            request_string[socket_bytes] = '\0';
        }
        //Siempre copia antes de parser, hay oportunidad de limpiar buffer para requests y response?
        strncpy(request_string,buffer,MAX_REQUEST_SIZE);
        status_code = parse_request(request_string, &request);

        
        /* La línea printf("Received request:\n%s\n", buffer); imprime en la consola el mensaje "Received request" seguido del contenido del buffer que contiene la solicitud recibida. */
        printf("Received request:\n%s\n", buffer);
        
        /*
        // Luego, se utiliza la función strstr() para buscar en la solicitud recibida si se trata de una solicitud GET, HEAD o POST. Dependiendo del resultado de esta búsqueda, se imprime en la consola un mensaje indicando el tipo de solicitud recibida. 
        // Parse and handle the request
        if (strstr(buffer, "GET") != NULL)
        {
            printf("GET request\n");
            // TODO: Handle GET request
        }
        else if (strstr(buffer, "HEAD") != NULL)
        {
            printf("HEAD request\n");
            // TODO: Handle HEAD request
        }
        else if (strstr(buffer, "POST") != NULL)
        {
            printf("POST request\n");
            // TODO: Handle POST request
        }
        else
        {
            printf("Unknown request\n");
        }
        */
        /* Después, se escribe en el socket la respuesta a la solicitud utilizando la función write(). En este caso, se envía un mensaje de respuesta HTTP simple que contiene el encabezado "HTTP/1.1 200 OK" seguido de los encabezados "Content-Type" y "Content-Length" y, finalmente, un mensaje de "Hello world!". */
        if (status_code == 200)
            socket_bytes = write(newsockfd, "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!", 74);
        else
            socket_bytes = write(newsockfd, "HTTP/1.1 400 Bad Request\r\n\r\n", 74);

        if (socket_bytes < 0)
            error("Error writing to socket\n");

        close(newsockfd);
    }

    close(sockfd);
    return 0;
}

// Imprime el error y sale
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

// De: https://stackoverflow.com/questions/20019786/safe-and-portable-way-to-convert-a-char-to-uint16-t
int str_to_uint16(char *str, uint16_t *res)
{
    char *end;
    int errno = 0;
    long val = strtol(str, &end, 10);
    if (errno || end == str || *end != '\0' || val < 0 || val >= 0x10000)
    {
        return -1;
    }
    *res = (uint16_t)val;
    return 0;
}

// La información necesaria para cada hilo
typedef struct thread_args
{
    int client_socket_fd;
    FILE *log_file;
    char *doc_root_folder;
} thread_args;

// FIXME TODO: Definir lista linkeada de headers. Quizas feo de trabajar pero no sabemos cuantos headers vamos a recibir. Ahí vemos como sale esta cosa.
typedef struct http_req_header
{
    char name[16]; // 16 es un valor inventado
    char value[16];
} http_req_header;



// TODO: Implementar una función que puede lidiar con un HEAD
int handle_HEAD(http_request *request, char *response)
{
    return 0;
}

// TODO: Implementar una función que puede lidiar con un GET
int handle_GET(http_request *request, char *response)
{
return 0;
}

// TODO: Implementar una función que puede lidiar con un POST
int handle_POST(http_request *request, char *response)
{
 return 0;
}

// TODO: Implementar la función para procesar la petición HTTP:
int process_request(http_request *request, char *response)
{
    printf("processing request");
    char file_path[256];
    FILE *file;
    int file_size, status_code;
    if (strcmp(request->method, "GET") != 0)
    {
        status_code = BAD_REQUEST;
        sprintf(response, "HTTP/1.1 %d Bad Request\r\n\r\n", status_code);
        return status_code;
    }
    sprintf(file_path, ".%s", request->path);
    file = fopen(file_path, "rb");
    if (file == NULL)
    {
        status_code = NOT_FOUND;
        sprintf(response, "HTTP/1.1 %d Not Found\r\n\r\n", status_code);
        return status_code;
    }
    fseek(file, 0L, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);
    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", file_size);
    fread(response + strlen(response), file_size, 1, file);
    fclose(file);
    return OK;
}

// TODO: Implementar la función para parsear la petición HTTP más allá del start line:
int parse_request(char *request_string, http_request *request)
{
    printf("parsing request");
    char *method, *path, *version;
    method = strtok(request_string, " ");
    if (method == NULL)
    {
        return BAD_REQUEST;
    }
    path = strtok(NULL, " ");
    if (path == NULL)
    {
        return BAD_REQUEST;
    }
    version = strtok(NULL, "\r\n");
    if (version == NULL)
    {
        return BAD_REQUEST;
    }
    strncpy(request->method, method, 8);
    strncpy(request->path, path, 256);
    strncpy(request->version, version, 16);
    return OK;
}

// Aquí se va a llamar socket(), bind(), y listen(). Asume IPv4 de momento.
int initialize_listening_socket(int http_port)
{
    // inicializa el socket

    /* AF_INET y SOCK_STREAM son dos constantes que se utilizan para especificar el tipo de socket que se desea crear en la función socket().
    AF_INET significa "Address Family - Internet", lo que indica que se utilizará la familia de protocolos de Internet para la comunicación. Es decir, el socket creado podrá ser utilizado para comunicación a través de IPv4 o IPv6.
    SOCK_STREAM significa que el socket será orientado a conexión y se utilizará el protocolo TCP (Transmission Control Protocol) para la transmisión de datos. Es decir, el socket creado será capaz de transmitir datos de manera confiable y ordenada, garantizando que los datos lleguen sin errores y en el mismo orden en que se enviaron. */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // si hay un error inicializando el socket
    if (sockfd < 0)
        error("Error opening socket\n");

    // Limpiar y configurar la estructura de direcciones del servidor

    //"serv_addr" es una estructura de tipo sockaddr_in que representa la dirección del socket en el lado del servidor. Esta estructura contiene información sobre el protocolo a utilizar, la dirección IP del servidor y el número de puerto que se está utilizando.
    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET, // IPv4
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(http_port), // host to network short
    };

    // hacer bind del socket
    /* La línea if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) verifica si la función bind() falló, lo que significa que no se pudo asociar el socket a la dirección y puerto especificados. En ese caso, la función error() se llama con un mensaje de error correspondiente. */
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("Error on binding\n");

    // poner el socket a escuchar
    /* La función listen(sockfd, CONNECTION_QUEUE_SIZE); indica que el socket creado en sockfd estará a la escucha de conexiones entrantes, y el número 5 indica el tamaño máximo de la cola de conexiones pendientes que el sistema operativo mantendrá para este socket.*/
    if (listen(sockfd, CONNECTION_QUEUE_SIZE) == -1)
        error("Error on listening\n");
    return sockfd;
}

/* FIXME handle_connection o como sea que se vaya a llamar ya tengo sueño son las 10 yisus christ...
void *serve(void *connection_info)
{
    thread_args thread_info = *(thread_args *)connection_info;
    int client_socket = thread_info.client_socket_fd;
    FILE *log_file = thread_info.log_file;

    char request_string[MAX_REQUEST_SIZE];
    int socket_bytes = read(client_socket, request_string, MAX_REQUEST_SIZE);
    if (socket_bytes < 0)
    {
        fprintf(log_file, "Error: read()\n");
        return NULL;
    }
    else
    {
        // null terminar el request string por sanidad
        request_string[socket_bytes] = '\0';
    }

    // FIXME parser de http
    char response_string[MAX_RESPONSE_SIZE];
    return NULL;
}
*/
