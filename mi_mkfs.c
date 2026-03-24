// mi_mkfs.c
// Descripción: Comando para crear y formatear el disco virtual. (Versión Nivel 2).

#include "ficheros_basico.h"
#include "directorios.h"

int main(int argc, char **argv)
{

    // Verificamos que los argumentos sean correctos
    // Uso: ./mi_mkfs <nombre_dispositivo> <nbloques>
    if (argc != 3)
    {
        fprintf(stderr, RED "Sintaxis incorrecta: %s <nombre_dispositivo> <nbloques>\n" RESET, argv[0]);
        return FALLO;
    }

    // Obtenemos los parámetros de la línea de comandos
    char *nombre_dispositivo = argv[1];
    int nbloques = atoi(argv[2]); // Convertimos el string de bloques a entero
    int ninodos = nbloques / 4;   // Heurística del enunciado

    // 1. Montamos el dispositivo virtual
    if (bmount(nombre_dispositivo) == FALLO)
    {
        fprintf(stderr, RED "Error: No se pudo montar el dispositivo '%s'\n" RESET, nombre_dispositivo);
        return FALLO;
    }

    // 2. Inicializamos el buffer con ceros
    unsigned char buffer[BLOCKSIZE];
    memset(buffer, 0, BLOCKSIZE);

    printf("Inicializando disco '%s' con %d bloques...\n", nombre_dispositivo, nbloques);

    // 3. Escribimos ceros en todos los bloques para limpiar el disco
    for (int i = 0; i < nbloques; i++)
    {
        if (bwrite(i, buffer) == FALLO)
        {
            fprintf(stderr, RED "Error al escribir en el bloque %d\n" RESET, i);
            bumount(); // Intentamos cerrar antes de salir
            return FALLO;
        }
    }

    if (sizeof(struct inodo) != 128)
    {
        fprintf(stderr, RED "Error: El tamaño del inodo es %lu y debe ser 128\n" RESET, sizeof(struct inodo));
        return FALLO;
    }
    // 4. Inicialización de metadatos (NIVEL 2)
    printf("Inicializando Superbloque...\n");
    if (initSB(nbloques, ninodos) == FALLO)
        return FALLO;

    printf("Inicializando Mapa de Bits...\n");
    if (initMB() == FALLO)
        return FALLO;

    printf("Inicializando Array de Inodos...\n");
    if (initAI() == FALLO)
        return FALLO;

    
    // --- NOVEDAD NIVEL 3: CREACIÓN DEL DIRECTORIO RAÍZ ---
    printf("Reservando inodo raiz...\n");
    // Reservamos inodo con tipo 'd' (directorio) y permisos 7 (rwx)
    if (reservar_inodo('d', 7) == FALLO) {
        fprintf(stderr, "Error al reservar inodo raiz\n");
        bumount();
        return FALLO;
    }

    // 5. Desmontamos el dispositivo
    if (bumount() == FALLO)
    {
        fprintf(stderr, RED "Error al desmontar el dispositivo\n" RESET);
        return FALLO;
    }

    printf(GREEN "Sistema de ficheros creado: %d bloques, %d inodos.\n" RESET, nbloques, ninodos);
    return EXITO;
}