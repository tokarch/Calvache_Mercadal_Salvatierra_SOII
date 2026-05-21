/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE POR
=====================================
Mercadal Forteza, Lluís
*/
// mi_touch.c
// Descripción: Comando para crear un fichero vacío con los permisos especificados.
// La ruta no debe acabar en '/' porque se trata de un fichero, no de un directorio.

#include "directorios.h"

/**
 * main (Comando mi_touch)
 * ------
 * Crea un fichero vacío en la ruta indicada con los permisos dados.
 * Uso: ./mi_touch <disco> <permisos> <ruta_fichero>
 * * Parámetros:
 *   - argv[1]: Nombre del dispositivo virtual.
 *   - argv[2]: Permisos en octal (0-7).
 *   - argv[3]: Ruta del fichero (no debe acabar en '/').
 * * Retorno:
 *   - EXITO (0) si se crea correctamente, FALLO (-1) en caso de error.
 * * Nota:
 *   - Los permisos van de 0 a 7 siguiendo el modelo octal estándar (rwx).
 *   - Un fichero no puede acabar en '/' para distinguirlo de un directorio.
 */
int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: %s <disco> <permisos> <ruta_fichero>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    int permisos = atoi(argv[2]);
    char *camino = argv[3];

    // Validamos que los permisos estén en el rango octal permitido (0-7)
    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, "Error: modo invalido: <<%d>>\n", permisos);
        return FALLO;
    }

    // Un fichero no puede acabar en '/' para distinguirlo de un directorio
    // Validación: fichero NO debe acabar en /
    if (camino[strlen(camino)-1] == '/') {
        fprintf(stderr, "Error: Es un fichero, no debe acabar en '/'\n");
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