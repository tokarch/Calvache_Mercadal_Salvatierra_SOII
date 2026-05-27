// mi_escribir.c
// Descripción: Comando para escribir en un fichero (usando ruta y offset).

#include "directorios.h"

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

/**
 * main (Comando mi_escribir)
 * ------
 * Programa encargado de escribir texto en un fichero del dispositivo virtual
 * a partir de un offset especificado, utilizando su ruta absoluta.
 * Verifica que la ruta corresponda a un fichero (no termine en '/') antes de intentar escribir.
 * Uso: ./mi_escribir <disco> <ruta_fichero> <texto> <offset>
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Ruta absoluta del fichero donde se va a escribir (ej. "/dir1/fichero.txt").
 * - argv[3]: Cadena de texto que se desea escribir.
 * - argv[4]: Offset o posición lógica a partir de la cual comenzar la escritura.
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: fprintf(), atoi(), strlen(), bmount(), printf(), mi_write(), mostrar_error_buscar_entrada(), bumount()
 */
int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta_fichero> <texto> <offset>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *camino = argv[2];
    char *texto = argv[3];
    unsigned int offset = atoi(argv[4]);

    // Comprobamos que no sea un directorio (no acabe en /)
    if (camino[strlen(camino) - 1] == '/') {
        fprintf(stderr, "Error: La ruta corresponde a un directorio.\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    printf("Longitud texto: %lu\n", strlen(texto));

    int bytes_escritos = mi_write(camino, texto, offset, strlen(texto));

    if (bytes_escritos < 0) {
        mostrar_error_buscar_entrada(bytes_escritos);
        bumount();
        return FALLO;
    }

    printf("Bytes escritos: %d\n", bytes_escritos);

    bumount();
    return EXITO;
}