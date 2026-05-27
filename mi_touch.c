#include "directorios.h"

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

/**
 * main (Comando mi_touch)
 * ------
 * Programa encargado de crear un nuevo fichero en el dispositivo virtual.
 * Verifica que los permisos sean válidos y que la ruta NO termine en '/'
 * antes de intentar crearlo.
 * Uso: ./mi_touch <disco> <permisos> <ruta_fichero>
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Permisos que tendrá el nuevo fichero (0-7).
 * - argv[3]: Ruta del fichero a crear, NO debe terminar en '/' (ej. "/dir1/fichero.txt").
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: bmount(), mi_creat(), mostrar_error_buscar_entrada(), bumount()
 */
int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: %s <disco> <permisos> <ruta_fichero>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    int permisos = atoi(argv[2]);
    char *camino = argv[3];

    if (permisos < 0 || permisos > 7) {
        fprintf(stderr, "Error: modo invalido: <<%d>>\n", permisos);
        return FALLO;
    }

    // Validación: fichero NO debe acabar en /
    if (camino[strlen(camino)-1] == '/') {
        fprintf(stderr, "Error: Es un fichero, no debe acabar en '/'\n");
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