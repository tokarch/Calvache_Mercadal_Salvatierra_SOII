// mi_link.c
// Descripción: Comando para crear enlaces físicos.

#include "directorios.h"

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

/**
 * main (Comando mi_link)
 * ------
 * Programa encargado de crear un enlace físico (hard link) a un fichero existente.
 * Verifica que ambas rutas correspondan a ficheros (no terminen en '/') antes de intentar
 * crear el enlace.
 * Uso: ./mi_link <disco> <ruta_fichero_original> <ruta_enlace>
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Ruta absoluta del fichero original ya existente (ej. "/dir1/fichero.txt").
 * - argv[3]: Ruta absoluta del nuevo enlace a crear (ej. "/dir1/enlace.txt").
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: fprintf(), strlen(), bmount(), mi_link(), mostrar_error_buscar_entrada(), bumount()
 */
int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta_fichero_original> <ruta_enlace>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *ruta_original = argv[2];
    char *ruta_enlace = argv[3];

    // Verificar que rutas no acaben en / (no enlaces a directorios)
    if (ruta_original[strlen(ruta_original)-1] == '/' || ruta_enlace[strlen(ruta_enlace)-1] == '/') {
        fprintf(stderr, "Error: Las rutas deben ser ficheros, no directorios.\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    int error = mi_link(ruta_original, ruta_enlace);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        bumount();
        return FALLO;
    }

    bumount();
    return EXITO;
}