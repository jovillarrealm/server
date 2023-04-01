# **server**

El punto es hacer un servidor en C con HTTP/1.1
Aquí es donde:

- Daniel
- Sara
- Jorge

Vamos a estar haciendo cosas.

******

## **Tabla de contenido**

1. [Inicio](#inicio)
2. [Estructura](#estructura)
3. [Miscelánea](#misc)

******

## **Inicio**

De momento se planea poder testear local, sabrá dios como, luego tener en una imagen de ubuntu de AWS esto corriendo, o algo, entonces en vez de lidiar con las mierdas de windows de no ser POSIX compliant con freaking sockets no es algo con lo que quiera lidiar.

## **Setup**

1. Prender un [WSL de ubuntu.](https://learn.microsoft.com/es-mx/windows/wsl/install) y [WSL en vscode.](https://code.visualstudio.com/docs/remote/wsl)
2. Desde wsl se usa con [Meson.](https://mesonbuild.com/SimpleStart.html)
3.

### **Detalles**

******

## **Estructura**

En un inicio se van a manejar peticiones de manera secuencial. Y en un mismo archivo.
Se va a tratar de implementar un threadpool para el manejo de cada conexión.
Se va a tratar de mantener el código modular.

## **Misc**

Joder con los archivos de tasks.json y launch.jason pareciera que fuera a dejar debuggear al menos un archivo, y el arhivo principal de un projecto de varios archivos.
