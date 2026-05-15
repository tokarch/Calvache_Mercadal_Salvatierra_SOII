// mi_link.c
// Descripción: Comando para crear enlaces físicos (equivalente a "ln" sin -s).
//              Un enlace físico es una nueva entrada de directorio que apunta
//              al mismo inodo (mismos datos y metadatos). Incrementa nlinks.
//              No se permiten enlaces a directorios para evitar ciclos en el árbol.

/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE Y COMENTADO POR
=====================================
Mercadal Forteza, Lluís
*/

#include "directorios.h"

/**
 * main (Comando mi_link)
 * ------
 * Crea un enlace físico (hard link) entre dos rutas del sistema de ficheros.
 * El enlace destino comparte el mismo inodo que el origen: ambos apuntan
 * a los mismos datos. Las rutas deben ser ficheros (no directorios) y no
 * pueden terminar en '/' para evitar ambigüedad semántica.
 *
 * * Parámetros (Línea de comandos):
 *   - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 *   - argv[2]: Ruta absoluta del fichero origen (debe existir).
 *   - argv[3]: Ruta absoluta del nuevo enlace (no debe existir).
 * * Retorno:
 *   - Devuelve EXITO (0) si se crea el enlace, o FALLO (-1) en caso de error.
 * * Llama a: bmount(), mi_link(), mostrar_error_buscar_entrada(), bumount()
 */
int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta_fichero_original> <ruta_enlace>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *ruta_original = argv[2];
    char *ruta_enlace = argv[3];

    // Verificar que rutas no acaben en '/' para evitar enlaces a directorios
    // (los enlaces a directorios podrían crear ciclos en el árbol de directorios)
    if (ruta_original[strlen(ruta_original)-1] == '/' || ruta_enlace[strlen(ruta_enlace)-1] == '/') {
        fprintf(stderr, "Error: Las rutas deben ser ficheros, no directorios.\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    // mi_link crea una nueva entrada de directorio que apunta al inodo origen,
    // incrementando su contador de enlaces (nlinks) sin duplicar datos
    int error = mi_link(ruta_original, ruta_enlace);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        bumount();
        return FALLO;
    }

    bumount();
    return EXITO;
}