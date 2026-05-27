// mi_rm.c
// Descripción: Comando para borrar ficheros o directorios.

#include "directorios.h"

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

/**
 * main (Comando mi_rm)
 * ------
 * Programa encargado de borrar un fichero o directorio del dispositivo virtual.
 * Elimina el enlace físico especificado por la ruta y, si se trata del último
 * enlace al inodo, libera sus bloques de datos y metadatos correspondientes.
 * Incluye una protección básica para evitar el borrado del directorio raíz ("/").
 * Uso: ./mi_rm <disco> <ruta>
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Ruta del fichero o directorio a borrar (ej. "/dir1/fichero.txt" o "/dir1/").
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: fprintf(), strcmp(), bmount(), mi_unlink(), mostrar_error_buscar_entrada(), bumount()
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *camino = argv[2];

    // Protección básica: no borrar la raíz
    if (strcmp(camino, "/") == 0) {
        fprintf(stderr, "Error: No se puede borrar el directorio raiz.\n");
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