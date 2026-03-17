// truncar.c
// Descripción: Programa externo para truncar ficheros.

#include "ficheros.h"

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "Sintaxis: %s <nombre_dispositivo> <ninodo> <nbytes>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    int ninodo = atoi(argv[2]);
    int nbytes = atoi(argv[3]);

    if (bmount(nombre_dispositivo) == FALLO)
        return FALLO;

    // Si nbytes es 0, liberamos el inodo completo
    if (nbytes == 0)
    {
        if (liberar_inodo(ninodo) == FALLO)
        {
            fprintf(stderr, "Error al liberar inodo %d\n", ninodo);
        }
        else
        {
            printf("Inodo %d liberado correctamente.\n", ninodo);
        }
    }
    // Si nbytes > 0, truncamos
    else
    {
        int liberados = mi_truncar_f(ninodo, nbytes);
        if (liberados == FALLO)
        {
            fprintf(stderr, "Error al truncar inodo %d\n", ninodo);
        }
        else
        {
            printf("Inodo %d truncado a %d bytes. Bloques liberados: %d\n", ninodo, nbytes, liberados);
        }
    }

    // Mostramos estado final (usando mi_stat_f si el inodo sigue activo)
    struct STAT stat;
    if (mi_stat_f(ninodo, &stat) == EXITO)
    {
        printf("\nDATOS INODO %d:\n", ninodo);
        printf("tamEnBytesLog=%d\n", stat.tamEnBytesLog);
        printf("numBloquesOcupados=%d\n", stat.numBloquesOcupados);
        printf("tipo=%c\n", stat.tipo);
    }
    else
    {
        // Si lo hemos liberado, stat podría fallar o devolver basura, o mostrar tipo 'l'
        printf("\n(El inodo %d parece haber sido liberado o no es accesible)\n", ninodo);
    }

    bumount();
    return EXITO;
}