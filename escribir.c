// escribir.c (Corregido)
#include "ficheros.h"

int main(int argc, char **argv)
{
    if (argc != 4)
    { // nombre, texto, ninodo
        // Nota: Según PDF "escribir <disco> <texto> <diferentes_inodos>"
        // Pero para simplificar y probar MI_WRITE, vamos a permitir pasar el inodo
        // o reservar uno si pasamos un flag.
        // Haremos algo simple: ./escribir disco "hola" 1 -> escribe en inodo 1 (reservándolo si hace falta)
        fprintf(stderr, "Sintaxis: %s <disco> <texto> <ninodo>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *texto = argv[2];
    int ninodo = atoi(argv[3]);

    if (bmount(nombre_dispositivo) == FALLO)
        return FALLO;

    // Reservamos el inodo si no es el raíz (y asumimos que está libre para el test)
    // En una prueba real, deberíamos comprobar si está libre, pero aquí forzamos la reserva
    // para asegurar que tiene permisos y fechas inicializadas.
    // OJO: Si ya está reservado, reservar_inodo devolverá otro ID.
    // Para simplificar este test unitario del Nivel 5:
    // Vamos a escribir directamente. Si el inodo no está inicializado (permisos 0), fallará.
    // Así que TRUCO: le damos permisos antes con mi_chmod_f o lo reservamos.

    // Vamos a simular lo que pide el PDF estrictamente: reservar un inodo nuevo.
    // Pero como quieres probar offsets específicos sobre un inodo concreto...
    // Haremos esto: Usamos mi_chmod_f para asegurar que el inodo 'ninodo' tiene permisos.
    mi_chmod_f(ninodo, 6); // rw-

    printf("Escribiendo en inodo %d...\n", ninodo);
    int longitud = strlen(texto);
    // Escribimos en 5 offsets diferentes para probar punteros directos e indirectos
    int offsets[] = {0, 5000, 200000, 40000000, 400000000};

    for (int i = 0; i < 5; i++)
    {
        printf("Escribiendo en offset %d...\n", offsets[i]);
        mi_write_f(ninodo, texto, offsets[i], longitud);
    }

    struct STAT stat;
    mi_stat_f(ninodo, &stat);
    printf("Stat final -> TamLogico: %d, BloquesOcupados: %d\n", stat.tamEnBytesLog, stat.numBloquesOcupados);

    bumount();
    return EXITO;
}