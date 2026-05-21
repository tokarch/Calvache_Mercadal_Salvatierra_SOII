// truncar.c
// Descripción: Programa externo para truncar ficheros.

#include "ficheros.h"

/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE POR
=====================================
Mercadal Forteza, Lluís
*/

/**
 * main (Comando truncar)
 * ------
 * Programa de usuario que permite reducir el tamaño de un fichero o liberarlo por completo.
 * Si se especifica un tamaño de 0 bytes, se invoca a liberar_inodo(). 
 * Si es superior a 0, se utiliza mi_truncar_f().
 * * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Número del inodo a truncar.
 * - argv[3]: Nuevo tamaño en bytes (nbytes).
 * * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * * Llama a: bmount(), liberar_inodo(), mi_truncar_f(), mi_stat_f(), ctime(), bumount()
 */
int main(int argc, char **argv)
{
    // 1. Verificar argumentos de la línea de comandos
    if (argc != 4)
    {
        fprintf(stderr, RED "Sintaxis: %s <nombre_dispositivo> <ninodo> <nbytes>\n" RESET, argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    int ninodo = atoi(argv[2]);
    int nbytes = atoi(argv[3]);

    // 2. Montar el dispositivo
    if (bmount(nombre_dispositivo) == FALLO)
        return FALLO;

    // Si nbytes es 0, probamos liberar_inodo() (eliminación completa)
    if (nbytes == 0)
    {
        liberar_inodo(ninodo);
    }
    // Si nbytes > 0, probamos mi_truncar_f() (truncado parcial)
    else
    {
        mi_truncar_f(ninodo, nbytes);
    }

    // 4. Mostramos estado final del inodo mediante mi_stat_f() para verificar cambios
    struct STAT stat;
    if (mi_stat_f(ninodo, &stat) != FALLO) {
        printf("\nDATOS DEL INODO %d:\n", ninodo);
        printf("tipo=%c\n", stat.tipo);
        printf("permisos=%d\n", stat.permisos);
        printf("atime: %s", ctime(&stat.atime));
        printf("mtime: %s", ctime(&stat.mtime));
        printf("ctime: %s", ctime(&stat.ctime));
        printf("nlinks=%d\n", stat.nlinks);
        printf("tamEnBytesLog=%d\n", stat.tamEnBytesLog);
        printf("numBloquesOcupados=%d\n", stat.numBloquesOcupados);
    } else {
        fprintf(stderr, RED "Error al obtener stat del inodo %d\n" RESET, ninodo);
    }

    // 5. Desmontar el dispositivo
    bumount();
    return EXITO;
}