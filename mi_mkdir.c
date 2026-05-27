#include "directorios.h"

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

/**
 * main (Comando mi_mkdir)
 * ------
 * Programa encargado de crear un nuevo directorio en el dispositivo virtual.
 * Verifica que la ruta termine en '/' y que los permisos sean válidos antes
 * de intentar crearlo.
 * Uso: ./mi_mkdir <disco> <permisos> <ruta_directorio/>
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Permisos que tendrá el nuevo directorio (0-7).
 * - argv[3]: Ruta del directorio a crear, debe terminar en '/' (ej. "/dir1/").
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: bmount(), mi_creat(), mostrar_error_buscar_entrada(), bumount()
 */
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