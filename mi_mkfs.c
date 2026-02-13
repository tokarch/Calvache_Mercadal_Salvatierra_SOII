// mi_mkfs.c
// Descripción: Comando para crear y formatear el disco virtual.

#include "bloques.h"

int main(int argc, char **argv) {
    
    // Verificamos que los argumentos sean correctos
    // Uso: ./mi_mkfs <nombre_dispositivo> <nbloques>
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis incorrecta: %s <nombre_dispositivo> <nbloques>\n" RESET, argv[0]);
        return FALLO;
    }

    // Obtenemos los parámetros de la línea de comandos
    char *nombre_dispositivo = argv[1];
    int nbloques = atoi(argv[2]); // Convertimos el string de bloques a entero

    // 1. Montamos el dispositivo virtual
    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, RED "Error: No se pudo montar el dispositivo '%s'\n" RESET, nombre_dispositivo);
        return FALLO;
    }

    // 2. Inicializamos el buffer con ceros
    unsigned char buffer[BLOCKSIZE];
    memset(buffer, 0, BLOCKSIZE);

    printf("Inicializando disco '%s' con %d bloques...\n", nombre_dispositivo, nbloques);

    // 3. Escribimos ceros en todos los bloques para limpiar el disco
    for (int i = 0; i < nbloques; i++) {
        if (bwrite(i, buffer) == FALLO) {
            fprintf(stderr, RED "Error al escribir en el bloque %d\n" RESET, i);
            bumount(); // Intentamos cerrar antes de salir
            return FALLO;
        }
    }

    // 4. Desmontamos el dispositivo
    if (bumount() == FALLO) {
        fprintf(stderr, RED "Error al desmontar el dispositivo\n" RESET);
        return FALLO;
    }

    printf(GREEN "Disco formateado correctamente con %d bloques de %d bytes.\n" RESET, nbloques, BLOCKSIZE);
    return EXITO;
}