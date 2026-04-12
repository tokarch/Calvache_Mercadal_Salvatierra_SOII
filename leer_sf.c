// leer_sf.c
// Descripción: Programa auxiliar para verificar la estructura del SF.

#include "ficheros_basico.h"

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

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, RED "Sintaxis: %s <nombre_dispositivo>\n" RESET, argv[0]);
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO)
        return FALLO;

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    // 1. Mostrar Superbloque
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

    // Verificación de tamaños
    printf("\nsizeof struct superbloque: %lu\n", sizeof(struct superbloque));
    printf("sizeof struct inodo: %lu\n", sizeof(struct inodo));

    // Recorrido de inodos libres para verificar lista enlazada
    printf("\nRECORRIDO LISTA INODOS LIBRES (Primeros 10)...\n");

    struct inodo inodo_aux;
    // 1. Empezamos exactamente donde el Superbloque dice que está el primero libre
    unsigned int inodo_actual = SB.posPrimerInodoLibre;

    int counter = 0;
    while (inodo_actual != UINT_MAX && counter < 10)
    {
        leer_inodo(inodo_actual, &inodo_aux);
        inodo_actual = inodo_aux.punterosDirectos[0]; // El siguiente libre está en el puntero directo 0
        printf("%d ", inodo_actual);                  // Imprimimos el inodo al que apunta
        counter++;
    }
    printf("\n");

    //// 2. Test Reserva/Liberación Bloque
    //printf("\nRESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS\n");
    //int nbloque = reservar_bloque();
    //bread(posSB, &SB); // Recargar SB para ver cambios
    //printf("Se ha reservado el bloque fisico n%d. Bloques libres: %d\n", nbloque, SB.cantBloquesLibres);

    //liberar_bloque(nbloque);
    //bread(posSB, &SB);
    //printf("Liberamos ese bloque. Bloques libres: %d\n", SB.cantBloquesLibres);

    //// 3. Test Lectura de Bits (Metadatos)
    //printf("\nMAPA DE BITS (Muestreo)\n");
    //printf("posSB (%d) -> leer_bit(%d) = %d\n", posSB, posSB, leer_bit(posSB));
    //printf("posPrimerBloqueMB (%d) -> leer_bit(%d) = %d\n", SB.posPrimerBloqueMB, SB.posPrimerBloqueMB, leer_bit(SB.posPrimerBloqueMB));
    //printf("posUltimoBloqueMB (%d) -> leer_bit(%d) = %d\n", SB.posUltimoBloqueMB, SB.posUltimoBloqueMB, leer_bit(SB.posUltimoBloqueMB));
    //printf("posPrimerBloqueAI (%d) -> leer_bit(%d) = %d\n", SB.posPrimerBloqueAI, SB.posPrimerBloqueAI, leer_bit(SB.posPrimerBloqueAI));
    //printf("posUltimoBloqueAI (%d) -> leer_bit(%d) = %d\n", SB.posUltimoBloqueAI, SB.posUltimoBloqueAI, leer_bit(SB.posUltimoBloqueAI));
    //printf("posPrimerBloqueDatos (%d) -> leer_bit(%d) = %d\n", SB.posPrimerBloqueDatos, SB.posPrimerBloqueDatos, leer_bit(SB.posPrimerBloqueDatos));
    //printf("posUltimoBloqueDatos (%d) -> leer_bit(%d) = %d\n", SB.posUltimoBloqueDatos, SB.posUltimoBloqueDatos, leer_bit(SB.posUltimoBloqueDatos));
    // 4. Test Directorio Raíz
    //struct inodo inodoRaiz;
    //if (leer_inodo(SB.posInodoRaiz, &inodoRaiz) == EXITO) {
    //    mostrar_inodo(&inodoRaiz, SB.posInodoRaiz);
    //} else {
    // printf("Error al leer el inodo raiz.\n");
    //}

    
    // --- TEST NIVEL 4 ---
    //printf("\nINODO 1. TRADUCCION DE LOS BLOQUES LOGICOS 8, 204, 30.004, 400.004 y 468.750\n");

    // 1. Reservamos un inodo artificialmente para pruebas (será el inodo 1)
    //int ninodo = reservar_inodo('f', 6);

    // 2. Traducimos bloques lógicos forzando reserva (reservar=1)
    // Esto creará la estructura de punteros
    //traducir_bloque_inodo(ninodo, 8, 1);
    //traducir_bloque_inodo(ninodo, 204, 1);
    //traducir_bloque_inodo(ninodo, 30004, 1);
    //traducir_bloque_inodo(ninodo, 400004, 1);
    //traducir_bloque_inodo(ninodo, 468750, 1);

    // 3. Mostramos el estado final del inodo
    //printf("\nDATOS DEL INODO RESERVADO %d\n", ninodo);
    //struct inodo inodo;
    //leer_inodo(ninodo, &inodo);
    //mostrar_inodo(&inodo, ninodo);

    // Mostrar SB final para ver consumo de bloques
    //bread(posSB, &SB);
    //printf("\nSB.posPrimerInodoLibre = %d\n", SB.posPrimerInodoLibre);

    if (bumount() == FALLO)
        return FALLO;
    return EXITO;
}