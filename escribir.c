// escribir.c
#include "ficheros.h"

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

/**
 * main (Comando escribir)
 * ------
 * Programa de usuario que escribe un texto pasado por argumento en un inodo específico.
 * Realiza las escrituras en varios offsets predefinidos (0, 5000, 200000, 40000000, 400000000) 
 * para forzar la traducción y reserva de bloques directos e indirectos de todos los niveles.
 * 
 * Hacemos writes en los offsets 0, 5000, 200000, 40000000 y 400000000.
 * - Offset 0:         Bloque Lógico 0   (Puntero Directo).
 * - Offset 5000:      Bloque Lógico 4   (Puntero Directo, pero obliga a usar desp1 y desp2).
 * - Offset 200000:    Bloque Lógico 195 (Indirecto Simple).
 * - Offset 40000000:  Bloque Lógico 39062 (Indirecto Doble).
 * - Offset 400000000: Bloque Lógico 390625 (Indirecto Triple).
 * Así probamos todas las ramas del árbol de inodos.
 *
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Texto o cadena de caracteres a escribir.
 * - argv[3]: Flag (0 = mismo inodo, 1 = diferentes inodos).
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: bmount(), reservar_inodo(), mi_write_f(), strlen(), mi_stat_f(), bumount()
 */
int main(int argc, char **argv)
{
    // Sintaxis estricta del PDF: escribir <nombre_dispositivo> <"texto"> <diferentes_inodos>
    if (argc != 4)
    {
        fprintf(stderr, RED "Sintaxis: %s <nombre_dispositivo> <\"texto\"> <diferentes_inodos>\n" RESET, argv[0]);
        fprintf(stderr, RED "Offsets: 9000, 209000, 30725000, 409605000, 480000000\n" RESET);
        fprintf(stderr, RED "Si diferentes_inodos=0 se reserva un solo inodo para todos los offsets\n" RESET);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *texto = argv[2];
    int diferentes_inodos = atoi(argv[3]); // 0 = mismo inodo, 1 = diferentes inodos

    if (bmount(nombre_dispositivo) == FALLO)
        return FALLO;

    int longitud = strlen(texto);
    printf("Longitud del texto: %d\n", longitud);

    // Offsets exactos que pide el script de prueba del Nivel 5
    int offsets[5] = {9000, 209000, 30725000, 409605000, 480000000};
    int ninodo;

    // Caso 1: Escribir todo en el mismo inodo
    if (diferentes_inodos == 0)
    {
        ninodo = reservar_inodo('f', 6); // 'f' de fichero, permisos 6 (rw-)
        if (ninodo == FALLO)
            return FALLO;

        for (int i = 0; i < 5; i++)
        {
            printf("\nNº inodo reservado: %d\n", ninodo);
            printf("offset: %d\n", offsets[i]);
            int escritos = mi_write_f(ninodo, texto, offsets[i], longitud);
            printf("Bytes escritos: %d\n", escritos);
            
            struct STAT stat;
            mi_stat_f(ninodo, &stat);
            printf("stat.tamEnBytesLog=%d\n", stat.tamEnBytesLog);
            printf("stat.numBloquesOcupados=%d\n", stat.numBloquesOcupados);
        }
    }
    // Caso 2: Escribir cada offset en un inodo diferente
    else if (diferentes_inodos == 1)
    {
        for (int i = 0; i < 5; i++)
        {
            ninodo = reservar_inodo('f', 6); // Reservamos un inodo nuevo en cada vuelta
            if (ninodo == FALLO)
                return FALLO;

            printf("\nNº inodo reservado: %d\n", ninodo);
            printf("offset: %d\n", offsets[i]);
            int escritos = mi_write_f(ninodo, texto, offsets[i], longitud);
            printf("Bytes escritos: %d\n", escritos);

            struct STAT stat;
            mi_stat_f(ninodo, &stat);
            printf("stat.tamEnBytesLog=%d\n", stat.tamEnBytesLog);
            printf("stat.numBloquesOcupados=%d\n", stat.numBloquesOcupados);
        }
    }

    bumount();
    return EXITO;
}