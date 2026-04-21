// mi_link.c
// Descripción: Comando para crear enlaces físicos.

#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta_fichero_original> <ruta_enlace>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *ruta_original = argv[2];
    char *ruta_enlace = argv[3];

    // Verificar que rutas no acaben en / (no enlaces a directorios)
    if (ruta_original[strlen(ruta_original)-1] == '/' || ruta_enlace[strlen(ruta_enlace)-1] == '/') {
        fprintf(stderr, "Error: Las rutas deben ser ficheros, no directorios.\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    int error = mi_link(ruta_original, ruta_enlace);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        bumount();
        return FALLO;
    }

    bumount();
    return EXITO;
}