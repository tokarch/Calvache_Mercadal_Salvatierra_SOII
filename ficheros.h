// ficheros.h
// Descripción: Prototipos y estructuras para la capa de ficheros (Nivel 5)

#ifndef FICHEROS_H
#define FICHEROS_H

#include "ficheros_basico.h"

// Estructura para devolver metadatos de un fichero (similar a stat de Linux)
struct STAT {
    unsigned char tipo;     // Tipo de fichero
    unsigned char permisos; // Permisos
    time_t atime;           // Último acceso
    time_t mtime;           // Última modificación
    time_t ctime;           // Último cambio de metadatos
    unsigned int nlinks;    // Número de enlaces
    unsigned int tamEnBytesLog; // Tamaño en bytes lógicos
    unsigned int numBloquesOcupados; // Bloques físicos ocupados
};

// Prototipos de funciones
int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes);
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes);
int mi_stat_f(unsigned int ninodo, struct STAT *p_stat);
int mi_chmod_f(unsigned int ninodo, unsigned char permisos);

#endif // FICHEROS_H