#include "directorios.h"

int main(int argc, char **argv) {
    int arg_idx = 1;

    if (argc == 4 && strcmp(argv[1], "-l") == 0) {
        arg_idx = 2;
    } else if (argc != 3) {
        fprintf(stderr, "Sintaxis: %s [-l] <disco> <ruta>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[arg_idx];
    char *camino = argv[arg_idx + 1];
    char buffer[TAMBUFFER];
    char tipo = 'd'; // Por defecto tratamos como directorio

    // Si no acaba en '/', es un fichero
    if (camino[strlen(camino)-1] != '/') {
        tipo = 'f';
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    // Limpiamos buffer
    memset(buffer, 0, TAMBUFFER);

    int total = mi_dir(camino, buffer, tipo);

    if (total < 0) {
        mostrar_error_buscar_entrada(total);
        bumount();
        return FALLO;
    }

    // Cabecera tipo 'ls -l'
    printf("Total: %d\n", total);
    if (total > 0) {
        printf("Tipo\tModo\tmTime\t\t\tTamaño\tNombre\n");
        printf("--------------------------------------------------------------------------------\n");
        printf("%s\n", buffer);
    }

    bumount();
    return EXITO;
}