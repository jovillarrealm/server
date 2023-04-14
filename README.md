# **Web Server In C**
## Telemática - 2023-I

# **Descripción del Proyecto**
En este proyecto se explora la aplicación de la capa de aplicación de la arquitectura
TCP/IP. Al respecto, se abordará desde el estudio del protocolo HTTP desde una
perspectiva de programación en red. Para esto se desarrollará e implementará un
servidor web.

**Integrantes**

Integrantes del equipo de trabajo:

- Daniel Gonzalez
- Sara Rodriguez Velasquez
- Jorge Alfredo Villareal

******

# **Tabla de contenido**

1. [Introducción](#introducción)
2. [Desarrollo](#desarrollo)
3. [Conclusiones](#conclusiones)
4. [Referencias](#referencias)

******

# **Introducción**

En este proyecto se explora la aplicación de la capa de aplicación de la arquitectura
TCP/IP. Al respecto, se abordará desde el estudio del protocolo HTTP desde una
perspectiva de programación en red. Para esto se desarrollará e implementará un
servidor web.

En términos generales la función principal de un servidor web es la entrega de recursos
(páginas html, imágenes, archivos de estilos, etc) web a un cliente que lo solicita (p.ej.,
web browser). Para esto, tanto el cliente como el servidor se comunican a través de un
mismo protocolo que es HTTP.

De esta forma el objetivo final es desarrollar e implementar un servidor web denominado
Telematics Web Server – (TWS)) que soporte la versión HTTP/1.1

******
# **Desarrollo**

## Descripción del código:

Actualmente se tiene un códifo de servidor web en C que acepta solicitudes HTTP. Recibe solicitudes HTTP de clientes y devuelve una respuesta HTTP en función de la solicitud. El servidor es un programa de consola que se ejecuta en un bucle infinito y espera nuevas conexiones de clientes. Se utiliza el protocolo de socket para establecer conexiones entre el cliente y el servidor. Se utiliza la biblioteca pthread.h para crear un hilo para cada conexión cliente. El servidor es capaz de manejar hasta un máximo de 15 conexiones simultáneas.

La solicitud HTTP se analiza utilizando la función parse_request_line que analiza la línea de solicitud HTTP para determinar el método, la ruta y el host. El servidor solo admite los métodos GET, POST, PUT y DELETE. Si se recibe una solicitud con un método diferente, se devuelve un código de respuesta HTTP 400 Bad Request.

El servidor tiene un registro (logger) que se guarda en un archivo que se especifica en el argumento de línea de comando. La función logger registra cada solicitud y respuesta HTTP en el archivo de registro con la marca de tiempo.

******

# **Conclusiones**



# **Referencias**

Se leyeron varios articulos y se visitaron varios archivos como referentes para el proyecto, los cuales se listan a continuación.

### Webgrafia y Bibliografia:

Video Tutorial "I made a web server in C like a true sigma". Imran Rahman. Disponible en youtube: https://www.youtube.com/watch?v=cEH_ipqHbUw&ab_channel=ImranRahman 

#### Referentes:

Ejemplo de un servidor sencillo en C. Comunidad de programadores. Disponible en: https://www.lawebdelprogramador.com/foros/C-Visual-C/1565635-Servidor-web-en-C.html 

Ejemplo de proyecto de servidor en C. Blooming Institute of Technology. Repositorio en Github. Disponible en: https://github.com/bloominstituteoftechnology/C-Web-Server 
