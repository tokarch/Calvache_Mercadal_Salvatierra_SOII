#include "directorios.h"

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

/**
 * main (Comando mi_rn)
 * ------
 * Programa encargado de renombrar un fichero dentro del sistema de ficheros virtual.
 * Uso: ./mi_rn <disco> <ruta_origen> <nuevo_nombre>
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Ruta del fichero de origen en el SF.
 * - argv[3]: Nuevo nombre del fichero en el mismo directorio.
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: bmount(), mi_rn(), mostrar_error_buscar_entrada(), bumount()
 */
int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta_origen> <nuevo_nombre>\n", argv[0]);
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO) return FALLO;

    int error = mi_rn(argv[2], argv[3]);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        bumount();
        return FALLO;
    }

    bumount();
    return EXITO;
}