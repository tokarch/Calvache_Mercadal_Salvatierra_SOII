// mi_rmdir.c
#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta_directorio>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *camino = argv[2];

    // Verificar que acaba en '/' (es directorio)
    // Nota: mi_unlink funciona igual, pero esto es una comprobación semántica de comando
    if (camino[strlen(camino)-1] != '/') {
        fprintf(stderr, "Error: La ruta debe acabar en '/' para ser un directorio.\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    int error = mi_unlink(camino);
    if (error < 0) {
        // Personalizamos mensaje si el error es genérico
        if (error == ERROR_ENTRADA_YA_EXISTENTE) // Reusado para dir no vacío
            fprintf(stderr, "Error: El directorio no esta vacio.\n");
        else
            mostrar_error_buscar_entrada(error);
        
        bumount();
        return FALLO;
    }

    bumount();
    return EXITO;
}