// leer_sf.c
// Descripción: Programa auxiliar para verificar la estructura del SF.

#include "ficheros_basico.h"

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

    // 5. Desmontar el dispositivo virtual
    if (bumount() == FALLO)
        return FALLO;
    return EXITO;
}