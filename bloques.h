// bloques.h
// Autor: [Tu Nombre/Grupo]
// Descripción: Definiciones globales y librerías básicas para el Sistema de Ficheros.

#ifndef BLOQUES_H
#define BLOQUES_H

#include <stdio.h>      // Entrada/Salida estándar
#include <stdlib.h>     // Funciones de memoria, conversión, etc.
#include <unistd.h>     // Llamadas al sistema POSIX (read, write, lseek, etc.)
#include <sys/types.h>  // Definición de tipos de datos (off_t, size_t)
#include <sys/stat.h>   // Estructuras para stat y permisos
#include <fcntl.h>      // Control de archivos (open, flags como O_RDWR)
#include <string.h>     // Manipulación de cadenas (memcpy, memset)
#include <errno.h>      // Gestión de errores

// ----------------------------------------------------------------------
// CONSTANTES DEL SISTEMA DE FICHEROS
// ----------------------------------------------------------------------

// Tamaño de bloque: El enunciado permite entre 512 y 4096 bytes.
// Usaremos 1024 como estándar equilibrado.
#define BLOCK_SIZE 1024 

// Mensajes de éxito/error (útil para depuración)
#define EXITO 0
#define FALLO -1

#endif // BLOQUES_H