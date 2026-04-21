#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: %s <disco> <permisos> <ruta_directorio/>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    int permisos = atoi(argv[2]);
    char *camino = argv[3];

    // Validación básica: permisos 0-7
    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, "Error: modo invalido: <<%d>>\n", permisos);
        return FALLO;
    }

    // Validación: debe ser directorio (acabar en /)
    if (camino[strlen(camino)-1] != '/') {
        fprintf(stderr, "Error: Es un directorio, debe acabar en '/'\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    int error = mi_creat(camino, (unsigned char)permisos);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        bumount();
        return FALLO;
    }

    bumount();
    return EXITO;
}