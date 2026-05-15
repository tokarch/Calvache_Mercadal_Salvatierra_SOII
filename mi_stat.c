/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE Y COMENTADO POR
=====================================
Mercadal Forteza, Lluís
*/

/*
 * mi_stat.c - Muestra metadatos de un fichero o directorio (equivalente a stat).
 * Uso: ./mi_stat <disco> <ruta>
 *   <disco>  : nombre del dispositivo virtual (ej. disco)
 *   <ruta>   : camino absoluto dentro del sistema de ficheros
 * Salida: campos del struct STAT del inodo asociado a la ruta.
 */
#include "directorios.h"

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

    /* Conversión de timestamps del struct STAT a cadena legible.
     * Se usa localtime (no localtime_r) porque la herramienta es monohilo. */
    struct tm *tm;
    char atime[80], mtime[80], ctime[80];
    
    tm = localtime(&stat.atime);
    sprintf(atime, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    tm = localtime(&stat.mtime);
    sprintf(mtime, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    tm = localtime(&stat.ctime);
    sprintf(ctime, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    /* Visualización de los campos del struct STAT del inodo:
     * tipo ('f' fichero / 'd' directorio), permisos (octal),
     * timestamps (atime/mtime/ctime), enlaces, tamaño lógico
     * y bloques físicos ocupados en el dispositivo. */
    printf("Nº de inodo: %d\n", ninodo);
    printf("tipo: %c\n", stat.tipo);
    printf("permisos: %d\n", stat.permisos);
    printf("atime: %s\n", atime);
    printf("mtime: %s\n", mtime);
    printf("ctime: %s\n", ctime);
    printf("nlinks: %d\n", stat.nlinks);
    printf("tamEnBytesLog: %d\n", stat.tamEnBytesLog);
    printf("numBloquesOcupados: %d\n", stat.numBloquesOcupados);

    bumount();
    return EXITO;
}