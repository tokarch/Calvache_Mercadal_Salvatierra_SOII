// mi_escribir.c
// Descripción: Comando para escribir en un fichero (usando ruta y offset).

#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta_fichero> <texto> <offset>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *camino = argv[2];
    char *texto = argv[3];
    unsigned int offset = atoi(argv[4]);

    // Comprobamos que no sea un directorio (no acabe en /)
    if (camino[strlen(camino) - 1] == '/') {
        fprintf(stderr, "Error: La ruta corresponde a un directorio.\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    printf("Longitud texto: %lu\n", strlen(texto));

    int bytes_escritos = mi_write(camino, texto, offset, strlen(texto));

    if (bytes_escritos < 0) {
        mostrar_error_buscar_entrada(bytes_escritos);
        bytes_escritos = 0;
    } else {
        printf("Bytes escritos: %d\n", bytes_escritos);
    }

    bumount();
    return EXITO;
}