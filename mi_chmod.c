/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE Y COMENTADO POR
=====================================
Mercadal Forteza, Lluís
*/
// mi_chmod.c
// Descripción: Comando para cambiar los permisos de un fichero o directorio existente.
// Los permisos se expresan en octal de 0 a 7 (combinación de lectura, escritura y ejecución).

#include "directorios.h"

/**
 * main (Comando mi_chmod)
 * ------
 * Modifica los permisos de un fichero o directorio existente.
 * Uso: ./mi_chmod <disco> <permisos> <ruta>
 * * Parámetros:
 *   - argv[1]: Nombre del dispositivo virtual.
 *   - argv[2]: Permisos en octal (0-7).
 *   - argv[3]: Ruta del fichero o directorio.
 * * Retorno:
 *   - EXITO (0) si se cambian correctamente, FALLO (-1) en caso de error.
 * * Nota:
 *   - El rango 0-7 sigue el modelo octal: 4 (lectura), 2 (escritura), 1 (ejecución).
 */
int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: %s <disco> <permisos> <ruta>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    int permisos = atoi(argv[2]);
    char *camino = argv[3];

    // Validamos que los permisos estén en el rango octal 0-7
    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, "Error: permisos invalidos\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    int error = mi_chmod(camino, (unsigned char)permisos);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        bumount();
        return FALLO;
    }

    bumount();
    return EXITO;
}