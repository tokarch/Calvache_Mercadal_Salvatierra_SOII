// directorios.h
// Descripción: Estructuras y funciones para la gestión de directorios (Nivel 10)

/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE Y COMENTADO POR
=====================================
Mercadal Forteza, Lluís
*/

#ifndef DIRECTORIOS_H
#define DIRECTORIOS_H

#include "ficheros.h"

// Tamaño del nombre de fichero/directorio
#define TAMNOMBRE 60
#define PROFUNDIDAD 32 // Profundidad máxima del árbol de directorios
#define TAMFILA 300 // Pongo 300 porque me daba warnings con 100, pero realmente con 100 debería ser suficiente para mostrar la info de un fichero/directorio. (Revisar)
#define TAMBUFFER (TAMFILA * 1000) // Buffer para mostrar ls (aprox 1000 entradas)

struct entrada
{
    char nombre[TAMNOMBRE]; // Nombre del fichero (60 bytes)
    unsigned int ninodo;    // Número de inodo asociado (4 bytes)
};
// Tamaño total struct entrada: 64 bytes.
// En un bloque de 1024 bytes caben 1024/64 = 16 entradas.

// Definición de errores específicos de directorios
#define ERROR_CAMINO_INCORRECTO -1
#define ERROR_PERMISO_LECTURA -2
#define ERROR_NO_EXISTE_ENTRADA -3
#define ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO -4
#define ERROR_PERMISO_ESCRITURA -5
#define ERROR_ENTRADA_YA_EXISTENTE -6
#define ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_FICHERO -7

// Prototipos
// Funciones Nivel 7
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo);
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos);
void mostrar_error_buscar_entrada(int error);

// Funciones Nivel 8
int mi_creat(const char *camino, unsigned char permisos);
int mi_dir(const char *camino, char *buffer, char tipo);
int mi_chmod(const char *camino, unsigned char permisos);
int mi_stat(const char *camino, struct STAT *p_stat);

// Funciones Nivel 9
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes);
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes);

// Funciones Nivel 10 (NUEVAS)
int mi_link(const char *camino1, const char *camino2);
int mi_unlink(const char *camino);

#endif // DIRECTORIOS_H