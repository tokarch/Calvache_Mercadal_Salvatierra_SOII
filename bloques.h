// bloques.h
// Descripción: Definiciones globales, librerías y prototipos de funciones de bajo nivel.

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

#ifndef BLOQUES_H
#define BLOQUES_H

#include <stdio.h>     // printf, fprintf, stderr, etc.
#include <stdlib.h>    // exit, malloc, atoi, etc.
#include <unistd.h>    // read, write, open, close, lseek
#include <sys/types.h> // Tipos de datos del sistema
#include <sys/stat.h>  // Permisos y flags de creación
#include <fcntl.h>     // O_RDWR, O_CREAT, O_TRUNC
#include <errno.h>     // Gestión de la variable errno
#include <string.h>    // strerror, memset

// Constantes del sistema
#define BLOCKSIZE 1024 // Tamaño del bloque en bytes
#define EXITO 0
#define FALLO -1

// Colores para la consola (útil para depuración visual)
#define BLACK "\x1B[30m"
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define WHITE "\x1B[37m"
#define ORANGE "\x1B[38;2;255;128;0m"
#define ROSE "\x1B[38;2;255;151;203m"
#define LBLUE "\x1B[38;2;53;149;240m"
#define LGREEN "\x1B[38;2;17;245;120m"
#define GRAY "\x1B[38;2;176;174;174m"
#define RESET "\x1b[0m"

#define NEGRITA "\x1b[1m"

// Prototipos de funciones de bloques.c
int bmount(const char *camino);
int bumount();
int bwrite(unsigned int nbloque, const void *buf);
int bread(unsigned int nbloque, void *buf);

#endif // BLOQUES_H