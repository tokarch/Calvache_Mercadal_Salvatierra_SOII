// truncar.c
// Descripción: Programa externo para truncar ficheros.

#include "ficheros.h"

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        fprintf(stderr, RED "Sintaxis: %s <nombre_dispositivo> <ninodo> <nbytes>\n" RESET, argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    int ninodo = atoi(argv[2]);
    int nbytes = atoi(argv[3]);

    if (bmount(nombre_dispositivo) == FALLO)
        return FALLO;

    // Si nbytes es 0, probamos liberar_inodo()
    if (nbytes == 0)
    {
        liberar_inodo(ninodo);
    }
    // Si nbytes > 0, probamos mi_truncar_f()
    else
    {
        mi_truncar_f(ninodo, nbytes);
    }

    // Mostramos estado final (usando mi_stat_f si el inodo sigue activo)
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

    bumount();
    return EXITO;
}