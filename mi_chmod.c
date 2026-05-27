#include "directorios.h"

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

/**
 * main (Comando mi_chmod)
 * ------
 * Programa encargado de cambiar los permisos de un fichero o directorio en el dispositivo virtual.
 * Uso: ./mi_chmod <disco> <permisos> <ruta>
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Nuevos permisos a asignar a la entrada (0-7).
 * - argv[3]: Ruta del fichero o directorio a modificar (ej. "/dir1/" o "/fichero.txt").
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: atoi(), bmount(), mi_chmod(), mostrar_error_buscar_entrada(), bumount()
 */
int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: %s <disco> <permisos> <ruta>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    int permisos = atoi(argv[2]);
    char *camino = argv[3];

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