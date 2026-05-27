// mi_rm_r.c
// Descripción: Comando para borrar recursivamente ficheros o directorios con contenido.

#include "directorios.h"

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

/**
 * borrar_recursivo
 * ------
 * Función recursiva que borra un fichero o un directorio y todo su contenido.
 * Si es un fichero, lo borra directamente. Si es un directorio, lee sus entradas,
 * se llama a sí misma para borrarlas y, una vez vacío, borra el propio directorio.
 * * Parámetros:
 * - camino: Ruta absoluta del fichero o directorio a borrar.
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: mi_stat(), mi_unlink(), mi_read(), strcpy(), strcat(), borrar_recursivo()
 * * Es llamada por: main(), borrar_recursivo()
 */
int borrar_recursivo(char *camino) {
    struct STAT stat;
    
    // 1. Obtenemos los metadatos de la ruta actual para saber si es fichero o directorio
    if (mi_stat(camino, &stat) < 0) return FALLO;

    // 2. Si es un fichero, lo borramos directamente y terminamos la recursión
    if (stat.tipo == 'f') {
        return mi_unlink(camino);
    } 
    // 3. Si es un directorio, iteramos sobre su contenido
    else if (stat.tipo == 'd') {
        struct entrada ent;
        
        // Mientras el directorio tenga contenido (tamEnBytesLog > 0)
        while (mi_stat(camino, &stat) != FALLO && stat.tamEnBytesLog > 0) {
            // 4. Leemos SIEMPRE la primera entrada (offset 0).
            // Como mi_unlink compacta el directorio, la entrada 1 pasa a ser la 0 tras cada borrado.
            if (mi_read(camino, &ent, 0, sizeof(struct entrada)) < 0) return FALLO;
            
            // 5. Construimos la ruta completa del hijo: camino_padre + nombre_entrada
            char camino_hijo[TAMNOMBRE * PROFUNDIDAD];
            strcpy(camino_hijo, camino);
            strcat(camino_hijo, ent.nombre);
            
            // 6. Consultamos el tipo del hijo para añadirle '/' si es directorio
            struct STAT stat_hijo;
            mi_stat(camino_hijo, &stat_hijo);
            if (stat_hijo.tipo == 'd') {
                strcat(camino_hijo, "/");
            }
            
            // 7. Llamada recursiva para borrar el hijo
            if (borrar_recursivo(camino_hijo) < 0) return FALLO;
        }
        // 8. Una vez el directorio está completamente vacío, lo borramos
        return mi_unlink(camino);
    }
    return EXITO;
}

/**
 * main (Comando mi_rm_r)
 * ------
 * Programa encargado de iniciar el borrado recursivo de un fichero o directorio del dispositivo virtual.
 * Uso: ./mi_rm_r <disco> <ruta>
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Ruta absoluta del fichero o directorio a borrar (ej. "/dir1/").
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: fprintf(), strcmp(), bmount(), borrar_recursivo(), mostrar_error_buscar_entrada(), bumount()
 */
int main(int argc, char **argv) {
    // 1. Verificación de argumentos
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: %s <disco> <ruta>\n", argv[0]);
        return FALLO;
    }
    
    // 2. Protección para no borrar el directorio raíz
    if (strcmp(argv[2], "/") == 0) {
        fprintf(stderr, "Error: No se puede borrar el directorio raiz.\n");
        return FALLO;
    }
    
    // 3. Montamos el dispositivo
    if (bmount(argv[1]) == FALLO) return FALLO;

    // 4. Iniciamos el borrado recursivo
    int error = borrar_recursivo(argv[2]);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        bumount();
        return FALLO;
    }
    
    // 5. Desmontamos el dispositivo
    bumount();
    return EXITO;
}