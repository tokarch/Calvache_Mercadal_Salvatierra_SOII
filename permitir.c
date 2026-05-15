// permitir.c
#include "ficheros.h"

/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE Y COMENTADO POR
=====================================
Mercadal Forteza, Lluís
*/

/**
 * main (Comando permitir)
 * ------
 * Programa de usuario que permite modificar los permisos de un inodo determinado.
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Número del inodo a modificar.
 * - argv[3]: Valor de los permisos a establecer (en formato numérico, ej. 7 para rwx).
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: bmount(), mi_chmod_f(), bumount()
 */
int main(int argc, char **argv)
{
    if (argc != 4)
    {
        fprintf(stderr, RED "Sintaxis: %s <nombre_dispositivo> <ninodo> <permisos>\n" RESET, argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    int ninodo = atoi(argv[2]);
    int permisos = atoi(argv[3]);

    if (bmount(nombre_dispositivo) == FALLO)
        return FALLO;

    if (mi_chmod_f(ninodo, (unsigned char)permisos) == EXITO)
    {
        printf("Permisos del inodo %d cambiados a %d\n", ninodo, permisos);
    }
    else
    {
        fprintf(stderr, RED "Error al cambiar permisos.\n" RESET);
    }

    bumount();
    return EXITO;
}