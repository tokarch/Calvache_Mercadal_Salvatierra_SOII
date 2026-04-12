// escribir.c
#include "ficheros.h"

/**
 * main (Comando escribir)
 * ------
 * Programa de usuario que escribe un texto pasado por argumento en un inodo específico.
 * Realiza las escrituras en varios offsets predefinidos (0, 5000, 200000, 40000000, 400000000) 
 * para forzar la traducción y reserva de bloques directos e indirectos de todos los niveles.
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
        fprintf(stderr, "Sintaxis: %s <nombre_dispositivo> <\"texto\"> <diferentes_inodos>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *texto = argv[2];
    int diferentes_inodos = atoi(argv[3]); // 0 = mismo inodo, 1 = diferentes inodos

    if (bmount(nombre_dispositivo) == FALLO)
        return FALLO;

    int longitud = strlen(texto);
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