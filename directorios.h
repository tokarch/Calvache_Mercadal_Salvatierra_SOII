// directorios.h
// Descripción: Estructuras y funciones para la gestión de directorios (Nivel 7)

#ifndef DIRECTORIOS_H
#define DIRECTORIOS_H

#include "ficheros.h"

// Tamaño del nombre de fichero/directorio
#define TAMNOMBRE 60 

struct entrada {
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
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo);
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos);

void mostrar_error_buscar_entrada(int error); // Función auxiliar para debug

#endif // DIRECTORIOS_H