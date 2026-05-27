// mi_cat.c
// Descripción: Comando para mostrar el contenido completo de un fichero.

#include "directorios.h"

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

#define TAM_BUFFER_LECTURA 1500 // Tamaño arbitrario para probar lecturas fragmentadas

/**
 * main (Comando mi_cat)
 * ------
 * Programa encargado de mostrar el contenido completo de un fichero del dispositivo virtual.
 * Lee secuencialmente el fichero indicado por su ruta absoluta y lo vuelca
 * por la salida estándar (pantalla) hasta llegar al final del fichero (EOF).
 * Verifica que la ruta no termine en '/' antes de intentar leer.
 * Uso: ./mi_cat <disco> <ruta_fichero>
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Ruta absoluta del fichero a leer (ej. "/dir1/fichero.txt").
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: fprintf(), strlen(), bmount(), memset(), mi_read(), write(), mostrar_error_buscar_entrada(), bumount()
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta_fichero>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *camino = argv[2];

    // Verificación básica de ruta
    if (camino[strlen(camino) - 1] == '/') {
        fprintf(stderr, "Error: La ruta corresponde a un directorio.\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    // Buffer de lectura
    unsigned char buffer[TAM_BUFFER_LECTURA];
    int offset = 0;
    int leidos = 0;
    int total_leidos = 0;

    memset(buffer, 0, TAM_BUFFER_LECTURA);

    // Bucle de lectura hasta fin de fichero
    while ((leidos = mi_read(camino, buffer, offset, TAM_BUFFER_LECTURA)) > 0) {
        write(1, buffer, leidos); // Escribir en stdout (file descriptor 1)
        
        total_leidos += leidos;
        offset += leidos;
        memset(buffer, 0, TAM_BUFFER_LECTURA);
    }

    // Gestionar errores si mi_read devolvió código negativo
    if (leidos < 0) {
        mostrar_error_buscar_entrada(leidos);
        bumount();
        return FALLO;
    }

    fprintf(stderr, "\nTotal bytes leídos: %d\n", total_leidos);

    bumount();
    return EXITO;
}