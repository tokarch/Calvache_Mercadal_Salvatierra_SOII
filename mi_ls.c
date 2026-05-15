/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE Y COMENTADO POR
=====================================
Mercadal Forteza, Lluís
*/
// mi_ls.c
// Descripción: Comando para listar el contenido de un directorio o la información de un fichero.
// Soporta el modo detallado (-l) para mostrar permisos, tamaño y fecha de modificación.

#include "directorios.h"

/**
 * main (Comando mi_ls)
 * ------
 * Lista el contenido de un directorio o muestra la información detallada de un fichero.
 * Uso: ./mi_ls [-l] <disco> <ruta>
 * * Parámetros:
 *   - argv[1]: "-l" opcional para listado detallado.
 *   - argv[arg_idx]: Nombre del dispositivo virtual.
 *   - argv[arg_idx + 1]: Ruta del directorio (acaba en '/') o fichero.
 * * Retorno:
 *   - EXITO (0) si se lista correctamente, FALLO (-1) en caso de error.
 * * Nota:
 *   - El carácter 'tipo' distingue directorios ('d') de ficheros ('f') porque
 *     mi_dir() necesita saber qué tipo de entrada buscar en la jerarquía.
 */
int main(int argc, char **argv) {
    // Desplazamos el índice de argumentos si se ha indicado la opción -l
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
    // Determinamos si la ruta apunta a un directorio o a un fichero.
    // Los directorios deben acabar en '/' para que buscar_entrada() los identifique correctamente.
    char tipo = 'd'; // Por defecto tratamos como directorio

    // Si no acaba en '/', es un fichero
    if (camino[strlen(camino)-1] != '/') {
        tipo = 'f';
    }

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    // Inicializamos el búfer donde mi_dir() volcará la lista formateada.
    // Es necesario limpiarlo para no mezclar datos residuales entre llamadas.
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