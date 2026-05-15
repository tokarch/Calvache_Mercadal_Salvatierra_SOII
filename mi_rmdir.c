// mi_rmdir.c
// Descripción: Comando para borrar directorios vacíos (equivalente a "rmdir").
//              Exige que la ruta termine en '/' para reforzar la semántica de directorio.
//              Rechaza el borrado si el directorio contiene entradas (error "no vacío").

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
 * main (Comando mi_rmdir)
 * ------
 * Elimina un directorio vacío del sistema de ficheros virtual.
 * La ruta debe terminar en '/' para distinguirla semánticamente de un fichero.
 * Si el directorio contiene entradas, se devuelve un error de "no vacío".
 *
 * * Parámetros (Línea de comandos):
 *   - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 *   - argv[2]: Ruta absoluta del directorio a borrar (debe acabar en '/').
 * * Retorno:
 *   - Devuelve EXITO (0) si se borra correctamente, o FALLO (-1) en caso de error.
 * * Llama a: bmount(), mi_unlink(), mostrar_error_buscar_entrada(), bumount()
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta_directorio>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *camino = argv[2];

    // Verificar que acaba en '/' para reforzar semánticamente que es un directorio
    // Nota: mi_unlink funciona igual, pero esto es una comprobación semántica de comando
    if (camino[strlen(camino)-1] != '/') {
        fprintf(stderr, "Error: La ruta debe acabar en '/' para ser un directorio.\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    // mi_unlink rechaza directorios no vacíos; reutilizamos ERROR_ENTRADA_YA_EXISTENTE
    int error = mi_unlink(camino);
    if (error < 0) {
        // Personalizamos mensaje si el error indica que el directorio tiene contenido
        if (error == ERROR_ENTRADA_YA_EXISTENTE) // Reusado para dir no vacío
            fprintf(stderr, "Error: El directorio no esta vacio.\n");
        else
            mostrar_error_buscar_entrada(error);
        
        bumount();
        return FALLO;
    }

    bumount();
    return EXITO;
}