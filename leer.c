// leer.c
#include "ficheros.h"

#define TAM_BUFFER 1500

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Sintaxis: %s <nombre_dispositivo> <ninodo>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    int ninodo = atoi(argv[2]);

    if (bmount(nombre_dispositivo) == FALLO)
        return FALLO;

    // Leer fichero completo
    char buffer[TAM_BUFFER];
    int offset = 0;
    int leidos = 0;
    int total_leidos = 0;

    memset(buffer, 0, TAM_BUFFER); // Limpiar buffer visualmente

    // Bucle de lectura
    while ((leidos = mi_read_f(ninodo, buffer, offset, TAM_BUFFER)) > 0)
    {
        write(1, buffer, leidos); // Escribir en salida estándar (pantalla)
        total_leidos += leidos;
        offset += leidos;
        memset(buffer, 0, TAM_BUFFER);
    }

    fprintf(stderr, "\nTotal bytes leidos: %d\n", total_leidos);

    // Verificación final con stat
    struct STAT stat;
    mi_stat_f(ninodo, &stat);
    fprintf(stderr, "Tamano en inodo: %d\n", stat.tamEnBytesLog);

    bumount();
    return EXITO;
}