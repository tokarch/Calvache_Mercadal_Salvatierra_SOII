################################################################################
#                   SISTEMA DE FICHEROS - SEGUNDA ENTREGA                      #
#                         NIVELES 1 AL 10 (INCLUIDO)                           #
################################################################################

AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni

DESCRIPCIÓN
===========
Segunda fase de la implementación del sistema de ficheros basado en inodos. 
En esta entrega se ha desarrollado la capa de directorios, permitiendo la 
creación de una jerarquía de ficheros y carpetas, la resolución de rutas 
(path resolution), el uso de cachés para optimizar accesos, y la gestión 
de enlaces físicos (hard links).

Niveles implementados en esta entrega:
7. Capa de directorios: Resolución de rutas (buscar_entrada) y extracción de caminos.
8. Comandos de usuario: Creación, listado y modificación de metadatos (mkdir, ls, chmod, stat).
9. Lectura y escritura con Caché de Directorios.
10. Enlaces físicos y borrado (link, unlink, rm, rmdir) con compactación de directorios.

MEJORAS IMPLEMENTADAS (PUNTUABLES)
==================================
1. Comando mi_ls extendido (Nivel 8): Se ha implementado el soporte para el flag '-l'. 
   Si se ejecuta sin el flag, muestra un listado simple. Si se ejecuta con '-l', 
   muestra la información detallada (tipo, permisos, mtime, tamaño, nombre).
2. Colores en mi_ls: El listado de directorios diferencia visualmente los ficheros 
   (verde) de los directorios (azul).
3. Separación de comandos de borrado: Se ha implementado 'mi_rm' para ficheros y 
   'mi_rmdir' para directorios, comprobando que los directorios estén vacíos antes 
   de permitir su borrado.

SINTAXIS DE LOS COMANDOS
========================
Compilación:
  $ make clean
  $ make (compila por defecto hasta el nivel 13)
NOTA: para los diferentes DEBUG hay que ir al Makefile y modificar los flags!!!!

Comandos de la entrega anterior:
  * mi_mkfs: ./mi_mkfs <nombre_dispositivo> <nbloques>
  * leer_sf: ./leer_sf <nombre_dispositivo>

Nuevos comandos de Directorios:
  * mi_mkdir: ./mi_mkdir <disco> <permisos> <ruta_directorio/>
  * mi_touch: ./mi_touch <disco> <permisos> <ruta_fichero>
  * mi_chmod: ./mi_chmod <disco> <permisos> <ruta>
  * mi_stat:  ./mi_stat <disco> <ruta>
  * mi_ls:    ./mi_ls [-l] <disco> <ruta>
  * mi_link:  ./mi_link <disco> <ruta_fichero_original> <ruta_enlace>
  * mi_rm:    ./mi_rm <disco> <ruta_fichero>
  * mi_rmdir: ./mi_rmdir <disco> <ruta_directorio/>

Comandos de Lectura/Escritura (Nivel 9):
  * mi_escribir: ./mi_escribir <disco> <ruta_fichero> <texto> <offset>
  * mi_cat:      ./mi_cat <disco> <ruta_fichero>
  * mi_escribir_varios: ./mi_escribir_varios <disco> <ruta_fichero> <texto> <offset>

RESTRICCIONES Y OBSERVACIONES
=============================
* La profundidad máxima del árbol de directorios está definida en PROFUNDIDAD (32).
* El tamaño máximo del nombre de un fichero/directorio es TAMNOMBRE (60 caracteres).
* No se permite la creación de enlaces físicos a directorios para evitar ciclos en el grafo.
* El borrado de una entrada en un directorio (unlink) compacta automáticamente el 
  directorio moviendo la última entrada al hueco dejado por la entrada borrada.