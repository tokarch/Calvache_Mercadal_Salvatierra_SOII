// mi_mkfs.c
// Descripción: Comando para crear y formatear el disco virtual. (Versión Nivel 2).

#include "ficheros_basico.h"
#include "directorios.h"

/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE POR
=====================================
Mercadal Forteza, Lluís
*/

/**
 * main (Comando mi_mkfs)
 * ------
 * Programa encargado de formatear e inicializar el dispositivo virtual.
 * Llena el disco de ceros, inicializa el Superbloque, el Mapa de Bits, 
 * el Array de Inodos y reserva el inodo correspondiente al directorio raíz.
 * Uso: ./mi_mkfs <nombre_dispositivo> <nbloques>
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Número total de bloques que tendrá el disco.
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: bmount(), memset(), bwrite(), initSB(), initMB(), initAI(), reservar_inodo(), bumount()
 */
int main(int argc, char **argv)
{

    // 1. Verificamos que los argumentos sean correctos
    // argc debe ser 3: argv[0]="mi_mkfs", argv[1]="nombre_dispositivo", argv[2]="nbloques"
    if (argc != 3)
    {
        fprintf(stderr, RED "Sintaxis incorrecta: %s <nombre_dispositivo> <nbloques>\n" RESET, argv[0]);
        return FALLO;
    }

    // atoi() nos sirve para obtener el valor numérico (int) a partir del string pasado en argv[2]
    char *nombre_dispositivo = argv[1];
    int nbloques = atoi(argv[2]);
    int ninodos = nbloques / 4;   // Heurística del enunciado para la cantidad de inodos

    // 2. Montamos el dispositivo virtual abriendo el fichero
    if (bmount(nombre_dispositivo) == FALLO)
    {
        fprintf(stderr, RED "Error: No se pudo montar el dispositivo '%s'\n" RESET, nombre_dispositivo);
        return FALLO;
    }

    // 3. Declaramos un búfer del tamaño de un bloque lógico
    // Usamos memset() de <string.h> para llenarlo completamente de ceros
    unsigned char buffer[BLOCKSIZE];
    memset(buffer, 0, BLOCKSIZE);

    printf("Inicializando disco '%s' con %d bloques...\n", nombre_dispositivo, nbloques);

    // 4. Escribimos ceros en todos los bloques para limpiar el disco
    // Iteramos tantas veces como 'nbloques' haya pasado el administrador
    // En cada iteración llamamos a bwrite() para escribir nuestro buffer de ceros, formateando el disco
    for (int i = 0; i < nbloques; i++)
    {
        if (bwrite(i, buffer) == FALLO)
        {
            fprintf(stderr, RED "Error al escribir en el bloque %d\n" RESET, i);
            bumount(); // Intentamos cerrar antes de salir
            return FALLO;
        }
    }

    // 5. Validamos por seguridad que la estructura inodo mida exactamente 128 bytes
    if (sizeof(struct inodo) != 128)
    {
        fprintf(stderr, RED "Error: El tamaño del inodo es %lu y debe ser 128\n" RESET, sizeof(struct inodo));
        return FALLO;
    }
    // 6. Inicialización de metadatos (NIVEL 2)
    printf("Inicializando Superbloque...\n");
    if (initSB(nbloques, ninodos) == FALLO)
        return FALLO;

    printf("Inicializando Mapa de Bits...\n");
    if (initMB() == FALLO)
        return FALLO;

    printf("Inicializando Array de Inodos...\n");
    if (initAI() == FALLO)
        return FALLO;

    // 7. Creación del directorio raíz (NIVEL 3)
    printf("Reservando inodo raiz...\n");
    // El inodo 0 siempre se reserva para el directorio raíz del sistema
    // Se inicializa con tipo 'd' (directorio) y permisos 7 (rwx en octal: lectura, escritura, ejecución)
    if (reservar_inodo('d', 7) == FALLO)
    {
        fprintf(stderr, RED "Error al reservar inodo raiz\n" RESET);
        bumount();
        return FALLO;
    }

    // 8. Desmontamos el dispositivo llamando a bumount() para cerrar el descriptor del fichero
    if (bumount() == FALLO)
    {
        fprintf(stderr, RED "Error al desmontar el dispositivo\n" RESET);
        return FALLO;
    }

    printf(GREEN "Sistema de ficheros creado: %d bloques, %d inodos.\n" RESET, nbloques, ninodos);
    return EXITO;
}