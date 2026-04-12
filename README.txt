################################################################################
#                   SISTEMA DE FICHEROS - PRIMERA ENTREGA                      #
#                         NIVELES 1 AL 6 (INCLUIDO)                            #
################################################################################

AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni

DESCRIPCIÓN
===========
Implementación de un sistema de ficheros basado en inodos sobre un dispositivo 
virtual (fichero en el sistema operativo anfitrión). Este proyecto abarca desde 
la gestión de bloques de bajo nivel hasta la liberación de bloques lógicos e 
inodos (truncado).

Niveles implementados:
1. Gestión de bloques (capa de bajo nivel).
2. Estructuración del dispositivo (SB, MB, AI, Datos).
3. Operaciones con el mapa de bits e inodos.
4. Direccionamiento de bloques (punteros directos e indirectos).
5. Operaciones de lectura y escritura de ficheros (capa de ficheros).
6. Liberación de bloques y truncado de ficheros.

MEJORAS IMPLEMENTADAS
=====================
1. Optimización de 'liberar_bloques_inodo()': Se ha implementado la versión 
   recursiva de la función para el tratamiento de bloques indirectos.
2. Sistema de Saltos: Se ha añadido lógica de optimización para saltar rangos 
   completos de bloques lógicos cuando se encuentran punteros a 0 o cuando el 
   rango completo queda fuera del truncado, evitando lecturas (breads) 
   innecesarias en disco.
3. Trazas de Depuración: Inclusión de directivas de precompilación (#if DEBUGN6) 
   para habilitar trazas detalladas de la liberación de bloques, facilitando la 
   verificación de la eficiencia del sistema.

SINTAXIS DE LOS COMANDOS
========================
Compilación:
  $ make clean
  $ make LEVEL=6 (cualquier otro número X generará los archivos para probar el nivel X) (por esto scripte1.sh tiene make LEVEL=6 en lugar de make)
Si se quieren cambiar los debugs, al inicio del Makefile estos (DEBUGNX) se pueden cambiar por 1 o 0.
El make LEVEL=7 esta a medias, pedimos ignorarlo porque el nivel 7 no se evalua en esta entrega

Comandos externos:
  * mi_mkfs: ./mi_mkfs <nombre_dispositivo> <nbloques>
  * escribir: ./escribir <nombre_dispositivo> <"texto"> <diferentes_inodos>
  * leer: ./leer <nombre_dispositivo> <ninodo>
  * truncar: ./truncar <nombre_dispositivo> <ninodo> <nbytes>
  * permitir: ./permitir <nombre_dispositivo> <ninodo> <permisos>
  * leer_sf: ./leer_sf <nombre_dispositivo>

RESTRICCIONES Y OBSERVACIONES
=============================
* Tamaño de bloque: 1024 bytes.
* Tamaño del inodo: 128 bytes.
* Número máximo de bloques lógicos: 16.843.020 (basado en niveles de indirección).
* El comando 'leer' vuelca el contenido a través de la salida estándar, mientras 
  que los metadatos y resúmenes se envían a stderr.
* El truncado a 0 bytes invoca automáticamente la liberación total del inodo.

ARCHIVOS INCLUIDOS
==================
- Capas: bloques.c/h, ficheros_basico.c/h, ficheros.c/h.
- Programas: mi_mkfs.c, leer_sf.c, escribir.c, leer.c, truncar.c, permitir.c.
- Otros: Makefile, texto2.txt, scripte1.sh y tests de nivel 6.