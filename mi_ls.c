// mi_ls.c
#include "directorios.h"

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

/**
 * main (Comando mi_ls)
 * ------
 * Programa encargado de listar el contenido de un directorio o información de un fichero.
 * Permite mostrar un listado simple o detallado mediante el uso del parámetro opcional '-l'.
 * Uso: ./mi_ls [-l] <disco> <ruta>
 * * Parámetros (Línea de comandos):
 * - argv[1] (opcional): Flag "-l" para habilitar el formato extendido.
 * - argv[1] o argv[2]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2] o argv[3]: Ruta del directorio o fichero a listar (ej. "/dir1/" o "/fichero.txt").
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: fprintf(), strcmp(), strlen(), bmount(), memset(), mi_dir(), mostrar_error_buscar_entrada(), printf(), bumount()
 */
int main(int argc, char **argv) {
    // Puede recibir 3 argumentos (sin -l) o 4 argumentos (con -l)
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Sintaxis: %s [-l] <disco> <ruta>\n", argv[0]);
        return FALLO;
    }

    int extendido = 0;
    char *nombre_dispositivo;
    char *camino;

    // Detectar si se ha pasado el argumento -l
    if (argc == 4) {
        if (strcmp(argv[1], "-l") == 0) {
            extendido = 1;
        } else {
            fprintf(stderr, "Sintaxis: %s [-l] <disco> <ruta>\n", argv[0]);
            return FALLO;
        }
        nombre_dispositivo = argv[2];
        camino = argv[3];
    } else {
        nombre_dispositivo = argv[1];
        camino = argv[2];
    }

    char buffer[TAMBUFFER];
    char tipo = 'd'; // Por defecto tratamos como directorio

    // Si no acaba en '/', es un fichero
    if (camino[strlen(camino)-1] != '/') {
        tipo = 'f';
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    memset(buffer, 0, TAMBUFFER);

    // Llamamos a mi_dir pasándole el flag 'extendido'
    int total = mi_dir(camino, buffer, tipo, extendido);

    if (total < 0) {
        mostrar_error_buscar_entrada(total);
        bumount();
        return FALLO;
    }

    printf("Total: %d\n", total);
    if (total > 0) {
        // Solo imprimimos la cabecera si es formato extendido
        if (extendido) {
            printf("Tipo\tModo\tmTime\t\t\tTamaño\tNombre\n");
            printf("--------------------------------------------------------------------------------\n");
        }
        printf("%s", buffer);
    }

    bumount();
    return EXITO;
}