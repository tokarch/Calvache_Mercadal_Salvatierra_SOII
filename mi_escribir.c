// mi_escribir.c
// Descripción: Comando para escribir texto en un fichero a partir de un offset dado.
//              Recibe 4 argumentos: dispositivo, ruta, texto y offset inicial.
//              Utiliza mi_write() para escribir los bytes y gestionar bloques directos/indirectos.

/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE POR
=====================================
Mercadal Forteza, Lluís
*/

#include "directorios.h"

/**
 * main (Comando mi_escribir)
 * ------
 * Escribe una cadena de texto en un fichero del sistema de ficheros virtual
 * a partir de un offset especificado. Si el fichero no existe, se crea.
 * El offset permite sobreescribir datos existentes o extender el fichero.
 *
 * * Parámetros (Línea de comandos):
 *   - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 *   - argv[2]: Ruta absoluta del fichero destino.
 *   - argv[3]: Texto a escribir (entre comillas si contiene espacios).
 *   - argv[4]: Offset inicial de escritura (bytes desde el inicio del fichero).
 * * Retorno:
 *   - Devuelve EXITO (0) si se escribe correctamente, o FALLO (-1) en caso de error.
 * * Llama a: bmount(), mi_write(), mostrar_error_buscar_entrada(), bumount()
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

    // Comprobamos que no sea un directorio (los ficheros no acaban en '/')
    if (camino[strlen(camino) - 1] == '/') {
        fprintf(stderr, "Error: La ruta corresponde a un directorio.\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    printf("Longitud texto: %lu\n", strlen(texto));

    // mi_write traduce la ruta a inodo, reserva bloques si es necesario
    // y escribe los bytes a partir del offset indicado
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