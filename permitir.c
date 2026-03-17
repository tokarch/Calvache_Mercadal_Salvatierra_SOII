// permitir.c
#include "ficheros.h"

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "Sintaxis: %s <nombre_dispositivo> <ninodo> <permisos>\n", argv[0]);
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
        fprintf(stderr, "Error al cambiar permisos.\n");
    }

    bumount();
    return EXITO;
}