#include "directorios.h"

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

/**
 * main (Comando mi_stat)
 * ------
 * Programa encargado de mostrar la información (metadatos) del inodo asociado a un fichero o directorio.
 * Muestra datos como el número de inodo, tipo, permisos, tiempos de acceso/modificación/creación, enlaces, tamaño y bloques.
 * Uso: ./mi_stat <disco> <ruta>
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Ruta del fichero o directorio a examinar (ej. "/dir1/" o "/fichero.txt").
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: bmount(), mi_stat(), mostrar_error_buscar_entrada(), localtime(), sprintf(), printf(), bumount()
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *camino = argv[2];
    struct STAT stat;

    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    int ninodo = mi_stat(camino, &stat);
    if (ninodo < 0) {
        mostrar_error_buscar_entrada(ninodo);
        bumount();
        return FALLO;
    }

    // Formateo de fechas
    struct tm *tm;
    char atime[80], mtime[80], ctime[80], btime[80];
    
    tm = localtime(&stat.atime);
    sprintf(atime, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    tm = localtime(&stat.mtime);
    sprintf(mtime, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    tm = localtime(&stat.ctime);
    sprintf(ctime, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    tm = localtime(&stat.btime);
    sprintf(btime, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    printf("Nº de inodo: %d\n", ninodo);
    printf("tipo: %c\n", stat.tipo);
    printf("permisos: %d\n", stat.permisos);
    printf("atime: %s\n", atime);
    printf("mtime: %s\n", mtime);
    printf("ctime: %s\n", ctime);
    printf("btime: %s\n", btime);
    printf("nlinks: %d\n", stat.nlinks);
    printf("tamEnBytesLog: %d\n", stat.tamEnBytesLog);
    printf("numBloquesOcupados: %d\n", stat.numBloquesOcupados);

    bumount();
    return EXITO;
}