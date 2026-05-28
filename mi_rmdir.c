// mi_rmdir.c
#include "directorios.h"

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

/**
 * main (Comando mi_rmdir)
 * ------
 * Programa encargado de borrar un directorio (que debe estar vacío) del dispositivo virtual.
 * Verifica semánticamente que la ruta proporcionada termina en '/' antes de proceder con el borrado.
 * Uso: ./mi_rmdir <disco> <ruta_directorio/>
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Ruta absoluta del directorio a eliminar (debe terminar en '/', ej. "/dir1/").
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: fprintf(), strlen(), bmount(), mi_unlink(), mostrar_error_buscar_entrada(), bumount()
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta_directorio>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *camino = argv[2];

    // Verificar que acaba en '/' (es directorio)
    // Nota: mi_unlink funciona igual, pero esto es una comprobación semántica de comando
    if (camino[strlen(camino)-1] != '/') {
        fprintf(stderr, "Error: La ruta debe acabar en '/' para ser un directorio.\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    int error = mi_unlink(camino);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        bumount();
        return FALLO;
    }

    bumount();
    return EXITO;
}