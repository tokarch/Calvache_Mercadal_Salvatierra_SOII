// mi_cat.c
// Descripción: Comando para mostrar el contenido completo de un fichero (equivalente a "cat").
//              Lee el fichero en bloques de TAM_BUFFER_LECTURA bytes mediante mi_read(),
//              actualizando el offset progresivamente hasta alcanzar el final del fichero.

/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE POR
=====================================
Mercadal Forteza, Lluís
*/

#include "directorios.h"

#define TAM_BUFFER_LECTURA 1500 // Tamaño arbitrario para probar lecturas fragmentadas

/**
 * main (Comando mi_cat)
 * ------
 * Lee secuencialmente todo el contenido de un fichero y lo vuelca
 * por la salida estándar (stdout). Utiliza un buffer de 1500 bytes
 * para demostrar lecturas fragmentadas: mi_read() puede devolver
 * menos bytes de los solicitados si llega al final del fichero.
 *
 * * Parámetros (Línea de comandos):
 *   - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 *   - argv[2]: Ruta absoluta del fichero a mostrar.
 * * Retorno:
 *   - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: bmount(), mi_read(), write(), mostrar_error_buscar_entrada(), bumount()
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta_fichero>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *camino = argv[2];

    // Verificación básica: los directorios terminan en '/', los ficheros no
    if (camino[strlen(camino) - 1] == '/') {
        fprintf(stderr, "Error: La ruta corresponde a un directorio.\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    // Buffer de lectura; 1500 bytes fuerza lecturas fragmentadas (bloque = 1024 bytes)
    unsigned char buffer[TAM_BUFFER_LECTURA];
    int offset = 0;
    int leidos = 0;
    int total_leidos = 0;

    memset(buffer, 0, TAM_BUFFER_LECTURA);

    // Bucle de lectura secuencial: mi_read() traduce offset a bloques físicos
    // y puede devolver menos bytes solicitados al llegar al final del fichero
    while ((leidos = mi_read(camino, buffer, offset, TAM_BUFFER_LECTURA)) > 0) {
        write(1, buffer, leidos); // Escribir en stdout (file descriptor 1)
        
        total_leidos += leidos;
        offset += leidos;         // Avanzar el offset para la siguiente lectura
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