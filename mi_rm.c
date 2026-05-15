// mi_rm.c
// Descripción: Comando para borrar ficheros o directorios (equivalente a "rm").
//              Utiliza mi_unlink() para eliminar la entrada de directorio.
//              Si el inodo llega a 0 enlaces físicos, libera sus bloques de datos.
//              Si hay enlaces restantes, solo se decrementa el contador nlinks.

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
 * main (Comando mi_rm)
 * ------
 * Borra un fichero o directorio del sistema de ficheros virtual.
 * Si es un directorio, debe estar vacío. Si es el último enlace físico
 * del inodo, se liberan todos sus bloques de datos y metadatos.
 *
 * * Parámetros (Línea de comandos):
 *   - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 *   - argv[2]: Ruta absoluta del fichero o directorio a borrar.
 * * Retorno:
 *   - Devuelve EXITO (0) si se borra correctamente, o FALLO (-1) en caso de error.
 * * Llama a: bmount(), mi_unlink(), mostrar_error_buscar_entrada(), bumount()
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *camino = argv[2];

    // Protección básica: no borrar la raíz porque es el anclaje del árbol de directorios
    if (strcmp(camino, "/") == 0) {
        fprintf(stderr, "Error: No se puede borrar el directorio raiz.\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    // mi_unlink elimina la entrada de directorio y decrementa nlinks del inodo;
    // si nlinks llega a 0, libera el inodo y todos sus bloques de datos
    int error = mi_unlink(camino);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        bumount();
        return FALLO;
    }

    bumount();
    return EXITO;
}