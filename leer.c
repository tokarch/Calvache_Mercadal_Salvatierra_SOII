// leer.c
#include "ficheros.h"

/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE POR
=====================================
Mercadal Forteza, Lluís
*/

#define TAM_BUFFER (BLOCKSIZE * 4)

/**
 * main (Comando leer)
 * ------
 * Programa de usuario que lee secuencialmente todo el contenido de un fichero (inodo)
 * y lo vuelca por la salida estándar (pantalla) hasta llegar al final del fichero (EOF).
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Número del inodo a leer.
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: bmount(), memset(), mi_read_f(), write(), mi_stat_f(), bumount()
 */
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Sintaxis: %s <nombre_dispositivo> <ninodo>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    int ninodo = atoi(argv[2]);

    if (bmount(nombre_dispositivo) == FALLO)
        return FALLO;

    // Leer fichero completo
    char buffer[TAM_BUFFER];
    int offset = 0;
    int leidos = 0;
    int total_leidos = 0;

    memset(buffer, 0, TAM_BUFFER); // Limpiar buffer visualmente

    // Bucle de lectura
    while ((leidos = mi_read_f(ninodo, buffer, offset, TAM_BUFFER)) > 0)
    {
        write(1, buffer, leidos); // Escribir en salida estándar (pantalla)
        total_leidos += leidos;
        offset += leidos;
        memset(buffer, 0, TAM_BUFFER);
    }

    if (leidos < 0)
    {
        bumount();
        return FALLO;
    }

    fprintf(stderr, "\nTotal bytes leidos: %d\n", total_leidos);

    // Verificación final con stat
    struct STAT stat;
    mi_stat_f(ninodo, &stat);
    fprintf(stderr, "Tamano en inodo: %d\n", stat.tamEnBytesLog);

    bumount();
    return EXITO;
}