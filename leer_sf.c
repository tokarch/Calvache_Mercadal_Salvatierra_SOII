// leer_sf.c
// Descripción: Programa auxiliar para verificar la estructura del SF.

#include "directorios.h"

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

/**
 * mostrar_inodo
 * -------------
 * Función auxiliar que muestra por pantalla los metadatos de un inodo formateando
 * las marcas de tiempo (time_t) a cadenas de texto legibles.
 * * Parámetros:
 * - inodo: puntero a la estructura del inodo que se quiere mostrar.
 * - ninodo: número de inodo (para imprimirlo en la cabecera del mensaje).
 */
void mostrar_inodo(struct inodo *inodo, int ninodo)
{
    struct tm *ts;
    char atime[80], mtime[80], ctime[80], btime[80];
    ts = localtime(&inodo->atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo->mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo->ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo->btime);
    strftime(btime, sizeof(btime), "%a %Y-%m-%d %H:%M:%S", ts);
    printf("\nDATOS DEL INODO %d:\n", ninodo);
    printf("tipo: %c\n", inodo->tipo);
    printf("permisos: %d\n", inodo->permisos);
    printf("atime: %s\n", atime);
    printf("mtime: %s\n", mtime);
    printf("ctime: %s\n", ctime);
    printf("btime: %s\n", btime);
    printf("nlinks: %d\n", inodo->nlinks);
    printf("tamEnBytesLog: %d\n", inodo->tamEnBytesLog);
    printf("numBloquesOcupados: %d\n", inodo->numBloquesOcupados);
}


/* mostrar_buscar_entrada
 * ----------------------
 * Función auxiliar para probar la función buscar_entrada con diferentes caminos y opciones de reserva.
 * Imprime el resultado o el error correspondiente.
 * * Parámetros:
 * - camino: ruta a buscar (ej. "/dir1/archivo.txt").
 * - reservar: 1 para intentar crear la entrada si no existe, 0 para solo buscar.
 */
void mostrar_buscar_entrada(char *camino, char reservar)
{
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;
    printf("\ncamino: %s, reservar: %d\n", camino, reservar);
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, reservar, 6)) < 0)
    {
        mostrar_error_buscar_entrada(error);
    }
    printf("**********************************************************************\n");
    return;
}

/**
 * main (Comando leer_sf)
 * ------
 * Programa auxiliar de usuario para verificar el estado y la estructura del sistema de ficheros.
 * Monta el dispositivo, lee el Superbloque (SB) y muestra por pantalla sus variables críticas.
 * Contiene además bloques de código (comentados mediante #if) útiles para debuggear los distintos niveles.
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: bmount(), bread(), bumount()
 */
int main(int argc, char **argv)
{
    // 1. Validar los argumentos de entrada
    if (argc != 2)
    {
        fprintf(stderr, RED "Sintaxis: %s <nombre_dispositivo>\n" RESET, argv[0]);
        return FALLO;
    }

    // 2. Montar el dispositivo virtual
    if (bmount(argv[1]) == FALLO)
        return FALLO;

    // 3.
    // Declaramos una estructura local Superbloque.
    // Usamos bread(posSB, &SB) para leer el bloque 0 entero desde el disco
    // y volcarlo directamente en la memoria de nuestra variable local SB
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    // 4. Mostrar por pantalla los datos del Superbloque
    // Si las fórmulas de tamMB y tamAI funcionaron correctamente,
    // posUltimoBloqueDatos deberá coincidir exactamente con (totBloques - 1)
    printf(YELLOW "DATOS DEL SUPERBLOQUE\n" RESET);
    printf("posPrimerBloqueMB = %d\n", SB.posPrimerBloqueMB);
    printf("posUltimoBloqueMB = %d\n", SB.posUltimoBloqueMB);
    printf("posPrimerBloqueAI = %d\n", SB.posPrimerBloqueAI);
    printf("posUltimoBloqueAI = %d\n", SB.posUltimoBloqueAI);
    printf("posPrimerBloqueDatos = %d\n", SB.posPrimerBloqueDatos);
    printf("posUltimoBloqueDatos = %d\n", SB.posUltimoBloqueDatos);
    printf("posInodoRaiz = %d\n", SB.posInodoRaiz);
    printf("posPrimerInodoLibre = %d\n", SB.posPrimerInodoLibre);
    printf("cantBloquesLibres = %d\n", SB.cantBloquesLibres);
    printf("cantInodosLibres = %d\n", SB.cantInodosLibres);
    printf("totBloques = %d\n", SB.totBloques);
    printf("totInodos = %d\n", SB.totInodos);

    // Bloques de DEBUG que se usaban en niveles anteriores
    /*
    #if DEBUGN2

    // Verificación de tamaños
    printf("\nsizeof struct superbloque: %lu\n", sizeof(struct superbloque));
    printf("sizeof struct inodo: %lu\n", sizeof(struct inodo));

    // Recorrido de inodos libres para verificar lista enlazada
    printf("\nRECORRIDO LISTA ENLAZADA DE INODOS LIBRES\n");

    struct inodo inodo_aux;
    unsigned int inodo_actual = SB.posPrimerInodoLibre;
    unsigned int total_libres = SB.cantInodosLibres;
    int i = 0;
    int puntos_suspensivos = 0;

    while (inodo_actual != UINT_MAX)
    {
        if (leer_inodo(inodo_actual, &inodo_aux) == FALLO) break;

        // Mostrar el campo punterosDirectos[0] de cada inodo
        if (i < 28 || (total_libres >= 19 && i >= (total_libres - 19))) {
            if (inodo_aux.punterosDirectos[0] == UINT_MAX) {
                printf("-1 ");
            } else {
                printf("%u ", inodo_aux.punterosDirectos[0]);
            }
        } else if (!puntos_suspensivos) {
            printf("... ");
            puntos_suspensivos = 1;
        }
        inodo_actual = inodo_aux.punterosDirectos[0];
        i++;
    }
    printf("\n");

    #endif

    #if DEBUGN3
    // 2. Test Reserva/Liberación Bloque
    printf("\nRESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS\n");
    int nbloque = reservar_bloque();
    bread(posSB, &SB); // Recargar SB para ver cambios
    printf("Se ha reservado el bloque fisico nº %d que era el primer libre indicado por el MB", nbloque);
    printf("SB.cantBloquesLibres = %d\n", SB.cantBloquesLibres);

    liberar_bloque(nbloque);
    bread(posSB, &SB);
    printf("Liberamos ese bloque y después SB.cantBloquesLibres = %d\n", SB.cantBloquesLibres);

    // 3. MAPA DE BITS CON BLOQUES DE METADATOS OCUPADOS
    printf("\nMAPA DE BITS CON BLOQUES DE METADATOS OCUPADOS\n");

    // Leemos el bit del bloque 0 (Superbloque)
    printf("posSB: %d → leer_bit(%d) = %d\n", posSB, posSB, leer_bit(posSB));

    // Leemos el primer bit del Mapa de Bits
    printf("SB.posPrimerBloqueMB: %d → leer_bit(%d) = %d\n", SB.posPrimerBloqueMB, SB.posPrimerBloqueMB, leer_bit(SB.posPrimerBloqueMB));

    // Leemos el último bit del Mapa de Bits
    printf("SB.posUltimoBloqueMB: %d → leer_bit(%d) = %d\n", SB.posUltimoBloqueMB, SB.posUltimoBloqueMB, leer_bit(SB.posUltimoBloqueMB));

    // Leemos el primer bit del Array de Inodos
    printf("SB.posPrimerBloqueAI: %d → leer_bit(%d) = %d\n", SB.posPrimerBloqueAI, SB.posPrimerBloqueAI, leer_bit(SB.posPrimerBloqueAI));

    // Leemos el último bit del Array de Inodos
    printf("SB.posUltimoBloqueAI: %d → leer_bit(%d) = %d\n", SB.posUltimoBloqueAI, SB.posUltimoBloqueAI, leer_bit(SB.posUltimoBloqueAI));

    // Leemos el primer bit de Datos (debería ser 0, está libre)
    printf("SB.posPrimerBloqueDatos: %d → leer_bit(%d) = %d\n", SB.posPrimerBloqueDatos, SB.posPrimerBloqueDatos, leer_bit(SB.posPrimerBloqueDatos));

    // Leemos el último bit de Datos (debería ser 0, está libre)
    printf("SB.posUltimoBloqueDatos: %d → leer_bit(%d) = %d\n", SB.posUltimoBloqueDatos, SB.posUltimoBloqueDatos, leer_bit(SB.posUltimoBloqueDatos));

    // 4. Test Directorio Raíz
    struct inodo inodoRaiz;
    if (leer_inodo(SB.posInodoRaiz, &inodoRaiz) == EXITO) {
        mostrar_inodo(&inodoRaiz, SB.posInodoRaiz);
    } else {
     printf(RED "Error al leer el inodo raiz.\n" RESET);
    }
    #endif

    #if DEBUGN4
    // --- TEST NIVEL 4 ---
    printf("\nINODO 1. TRADUCCION DE LOS BLOQUES LOGICOS 8, 204, 30.004, 400.004 y 468.750\n");

    // 1. Reservamos un inodo artificialmente para pruebas (será el inodo 1)
    int ninodo = reservar_inodo('f', 6);

    // 2. Traducimos bloques lógicos forzando reserva (reservar=1)
    // Esto creará la estructura de punteros
    traducir_bloque_inodo(ninodo, 8, 1);
    printf("\n");
    traducir_bloque_inodo(ninodo, 204, 1);
    printf("\n");
    traducir_bloque_inodo(ninodo, 30004, 1);
    printf("\n");
    traducir_bloque_inodo(ninodo, 400004, 1);
    printf("\n");
    traducir_bloque_inodo(ninodo, 468750, 1);

    // 3. Mostramos el estado final del inodo
    printf("\nDATOS DEL INODO RESERVADO %d\n", ninodo);
    struct inodo inodo;
    leer_inodo(ninodo, &inodo);
    mostrar_inodo(&inodo, ninodo);

    // Mostrar SB final para ver consumo de bloques
    bread(posSB, &SB);
    printf("\nSB.posPrimerInodoLibre = %d\n", SB.posPrimerInodoLibre);
    #endif
    */

    // --- TEST NIVEL 7 ---

    /*
    mostrar_buscar_entrada("pruebas/", 1);           // ERROR_CAMINO_INCORRECTO
    mostrar_buscar_entrada("/pruebas/", 0);          // ERROR_NO_EXISTE_ENTRADA_CONSULTA
    mostrar_buscar_entrada("/pruebas/docs/", 1);     // ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO
    mostrar_buscar_entrada("/pruebas/", 1);          // creamos /pruebas/
    mostrar_buscar_entrada("/pruebas/docs/", 1);     // creamos /pruebas/docs/
    mostrar_buscar_entrada("/pruebas/docs/doc1", 1); // creamos /pruebas/docs/doc1
    mostrar_buscar_entrada("/pruebas/docs/doc1/doc11", 1);
    // ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO
    mostrar_buscar_entrada("/pruebas/", 1);          // ERROR_ENTRADA_YA_EXISTENTE
    mostrar_buscar_entrada("/pruebas/docs/doc1", 0); // consultamos /pruebas/docs/doc1
    mostrar_buscar_entrada("/pruebas/docs/doc1", 1); // ERROR_ENTRADA_YA_EXISTENTE
    mostrar_buscar_entrada("/pruebas/casos/", 1);    // creamos /pruebas/casos/
    mostrar_buscar_entrada("/pruebas/docs/doc2", 1); // creamos /pruebas/docs/doc2
    */

    // 5. Desmontar el dispositivo virtual
    if (bumount() == FALLO)
        return FALLO;
    return EXITO;
}