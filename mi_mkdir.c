/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE Y COMENTADO POR
=====================================
Mercadal Forteza, Lluís
*/
// mi_mkdir.c
// Descripción: Comando para crear un directorio con los permisos especificados.
// La ruta debe acabar en '/' para que el sistema identifique que se trata de un directorio.

#include "directorios.h"

/**
 * main (Comando mi_mkdir)
 * ------
 * Crea un directorio en la ruta indicada con los permisos dados.
 * Uso: ./mi_mkdir <disco> <permisos> <ruta_directorio/>
 * * Parámetros:
 *   - argv[1]: Nombre del dispositivo virtual.
 *   - argv[2]: Permisos en octal (0-7).
 *   - argv[3]: Ruta del directorio (debe acabar en '/').
 * * Retorno:
 *   - EXITO (0) si se crea correctamente, FALLO (-1) en caso de error.
 * * Nota:
 *   - El carácter final '/' es obligatorio para distinguir un directorio de un fichero.
 */
int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: ./mi_mkdir <nombre_dispositivo> <permisos> </ruta_directorio/>.\n");
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    int permisos = atoi(argv[2]);
    char *camino = argv[3];

    // Validamos que los permisos estén en el rango octal permitido (0-7)
    // Validación básica: permisos 0-7
    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, "Error: modo invalido: <<%d>>\n", permisos);
        return FALLO;
    }

    // Un directorio debe acabar en '/' para que el sistema lo identifique como tal
    // Validación: debe ser directorio (acabar en /)
    if (camino[strlen(camino)-1] != '/') {
        fprintf(stderr, "Error: Camino incorrecto.\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    int error = mi_creat(camino, (unsigned char)permisos);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        bumount();
        return FALLO;
    }

    bumount();
    return EXITO;
}