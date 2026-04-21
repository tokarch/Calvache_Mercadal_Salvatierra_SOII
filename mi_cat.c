// mi_cat.c
// Descripción: Comando para mostrar el contenido completo de un fichero.

#include "directorios.h"

#define TAM_BUFFER_LECTURA 1500 // Tamaño arbitrario para probar lecturas fragmentadas

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta_fichero>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *camino = argv[2];

    // Verificación básica de ruta
    if (camino[strlen(camino) - 1] == '/') {
        fprintf(stderr, "Error: La ruta corresponde a un directorio.\n");
        return FALLO;
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    // Buffer de lectura
    unsigned char buffer[TAM_BUFFER_LECTURA];
    int offset = 0;
    int leidos = 0;
    int total_leidos = 0;

    memset(buffer, 0, TAM_BUFFER_LECTURA);

    // Bucle de lectura hasta fin de fichero
    while ((leidos = mi_read(camino, buffer, offset, TAM_BUFFER_LECTURA)) > 0) {
        write(1, buffer, leidos); // Escribir en stdout (file descriptor 1)
        
        total_leidos += leidos;
        offset += leidos;
        memset(buffer, 0, TAM_BUFFER_LECTURA);
    }

    // Gestionar errores si mi_read devolvió código negativo
    if (leidos < 0) {
        mostrar_error_buscar_entrada(leidos);
    } else {
        fprintf(stderr, "\nTotal bytes leídos: %d\n", total_leidos);
    }

    bumount();
    return EXITO;
}