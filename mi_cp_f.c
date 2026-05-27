#include "directorios.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

/**
 * main (Comando mi_cp_f)
 * ------
 * Programa encargado de copiar un fichero de origen a un destino dentro del
 * sistema de ficheros virtual.
 * Uso: ./mi_cp_f <disco> <origen> <destino>
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Ruta del fichero de origen en el SF (ej. "/pruebas/origen.txt").
 * - argv[3]: Ruta destino o directorio destino (ej. "/pruebas/destino/").
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: bmount(), mi_cp_f(), bumount()
 */
int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: %s <disco> <origen> <destino>\n", argv[0]);
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO) return FALLO;

    if (mi_cp_f(argv[2], argv[3]) < 0) {
        bumount();
        return FALLO;
    }

    bumount();
    return EXITO;
}