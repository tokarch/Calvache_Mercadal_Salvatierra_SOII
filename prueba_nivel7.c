// prueba_nivel7.c
// Descripción: Programa para testear extraer_camino y buscar_entrada.
// Propósito: Test harness de integración del Nivel 7 (resolución de caminos
//   y gestión de entradas de directorio). Comprueba creación, búsqueda y
//   detección de duplicados sobre un dispositivo virtual montado.
// Uso: ./prueba_nivel7 <nombre_dispositivo>

/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE Y COMENTADO POR
=====================================
Mercadal Forteza, Lluís
*/

#include "directorios.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Sintaxis: %s <nombre_dispositivo>\n", argv[0]);
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO)
        return FALLO;

    /* 1. Test de creación de directorios y ficheros.
     * buscar_entrada con reservar=1 crea la entrada si no existe.
     * El último argumento son los permisos en octal (7=rwx, 6=rw-). */
    printf("\n--- TEST CREACION ---\n");
    unsigned int p_inodo_dir = 0; // Empezamos en la raíz
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;

    // Crear /docs
    printf("Creando /docs ... ");
    error = buscar_entrada("/docs", &p_inodo_dir, &p_inodo, &p_entrada, 1, 7); // 7 = rwx
    if (error < 0)
        mostrar_error_buscar_entrada(error);
    else
        printf("OK (Inodo: %d)\n", p_inodo);

    // Crear /docs/trabajo (dentro del anterior)
    p_inodo_dir = 0; // Reset a raíz para búsqueda completa
    printf("Creando /docs/trabajo ... ");
    error = buscar_entrada("/docs/trabajo", &p_inodo_dir, &p_inodo, &p_entrada, 1, 7);
    if (error < 0)
        mostrar_error_buscar_entrada(error);
    else
        printf("OK (Inodo: %d)\n", p_inodo);

    // Crear /docs/trabajo/informe.txt (fichero)
    p_inodo_dir = 0;
    printf("Creando /docs/trabajo/informe.txt ... ");
    error = buscar_entrada("/docs/trabajo/informe.txt", &p_inodo_dir, &p_inodo, &p_entrada, 1, 6); // 6 = rw-
    if (error < 0)
        mostrar_error_buscar_entrada(error);
    else
        printf("OK (Inodo: %d)\n", p_inodo);

    /* 2. Test de búsqueda de entradas existentes y no existentes.
     * buscar_entrada con reservar=0 solo consulta; devuelve el inodo
     * y el índice de entrada si lo encuentra, o un código de error negativo. */
    printf("\n--- TEST BUSQUEDA ---\n");

    // Buscar /docs/trabajo/informe.txt
    p_inodo_dir = 0;
    printf("Buscando /docs/trabajo/informe.txt ... ");
    error = buscar_entrada("/docs/trabajo/informe.txt", &p_inodo_dir, &p_inodo, &p_entrada, 0, 6);
    if (error < 0)
        mostrar_error_buscar_entrada(error);
    else
        printf("ENCONTRADO (Inodo: %d, Entrada Index: %d)\n", p_inodo, p_entrada);

    // Buscar algo que no existe
    p_inodo_dir = 0;
    printf("Buscando /docs/noexiste ... ");
    error = buscar_entrada("/docs/noexiste", &p_inodo_dir, &p_inodo, &p_entrada, 0, 6);
    if (error < 0)
        printf("OK (Error esperado detectado)\n");
    else
        printf("ERROR (Encontrado algo que no deberia existir)\n");

    /* 3. Test de detección de entrada duplicada.
     * Si buscar_entrada con reservar=1 encuentra la entrada previa,
     * debe devolver ERROR_ENTRADA_YA_EXISTENTE. */
    p_inodo_dir = 0;
    printf("Intentando crear /docs (ya existe) ... ");
    error = buscar_entrada("/docs", &p_inodo_dir, &p_inodo, &p_entrada, 1, 7);
    if (error == ERROR_ENTRADA_YA_EXISTENTE)
        printf("OK (Error 'Ya existe' detectado)\n");
    else
        printf("FALLO (No detecto existencia previa)\n");

    bumount();
    return EXITO;
}