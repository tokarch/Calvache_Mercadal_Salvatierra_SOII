// prueba_nivel7.c
// Descripción: Programa para testear extraer_camino y buscar_entrada.

#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Sintaxis: %s <nombre_dispositivo>\n", argv[0]);
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO) return FALLO;

    // 1. Test de creación de directorios
    printf("\n--- TEST CREACION ---\n");
    unsigned int p_inodo_dir = 0; // Empezamos en la raíz
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;

    // Crear /docs
    printf("Creando /docs ... ");
    error = buscar_entrada("/docs", &p_inodo_dir, &p_inodo, &p_entrada, 1, 7); // 7 = rwx
    if (error < 0) mostrar_error_buscar_entrada(error);
    else printf("OK (Inodo: %d)\n", p_inodo);

    // Crear /docs/trabajo (dentro del anterior)
    p_inodo_dir = 0; // Reset a raíz para búsqueda completa
    printf("Creando /docs/trabajo ... ");
    error = buscar_entrada("/docs/trabajo", &p_inodo_dir, &p_inodo, &p_entrada, 1, 7);
    if (error < 0) mostrar_error_buscar_entrada(error);
    else printf("OK (Inodo: %d)\n", p_inodo);

    // Crear /docs/trabajo/informe.txt (fichero)
    p_inodo_dir = 0;
    printf("Creando /docs/trabajo/informe.txt ... ");
    error = buscar_entrada("/docs/trabajo/informe.txt", &p_inodo_dir, &p_inodo, &p_entrada, 1, 6); // 6 = rw-
    if (error < 0) mostrar_error_buscar_entrada(error);
    else printf("OK (Inodo: %d)\n", p_inodo);

    // 2. Test de búsqueda
    printf("\n--- TEST BUSQUEDA ---\n");
    
    // Buscar /docs/trabajo/informe.txt
    p_inodo_dir = 0;
    printf("Buscando /docs/trabajo/informe.txt ... ");
    error = buscar_entrada("/docs/trabajo/informe.txt", &p_inodo_dir, &p_inodo, &p_entrada, 0, 6);
    if (error < 0) mostrar_error_buscar_entrada(error);
    else printf("ENCONTRADO (Inodo: %d, Entrada Index: %d)\n", p_inodo, p_entrada);

    // Buscar algo que no existe
    p_inodo_dir = 0;
    printf("Buscando /docs/noexiste ... ");
    error = buscar_entrada("/docs/noexiste", &p_inodo_dir, &p_inodo, &p_entrada, 0, 6);
    if (error < 0) printf("OK (Error esperado detectado)\n");
    else printf("ERROR (Encontrado algo que no deberia existir)\n");

    // 3. Test crear existente
    p_inodo_dir = 0;
    printf("Intentando crear /docs (ya existe) ... ");
    error = buscar_entrada("/docs", &p_inodo_dir, &p_inodo, &p_entrada, 1, 7);
    if (error == ERROR_ENTRADA_YA_EXISTENTE) printf("OK (Error 'Ya existe' detectado)\n");
    else printf("FALLO (No detecto existencia previa)\n");


    bumount();
    return EXITO;
}