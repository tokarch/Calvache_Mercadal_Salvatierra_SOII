// leer_sf.c
// Descripción: Programa auxiliar para verificar la estructura del SF.

#include "ficheros_basico.h"

void mostrar_inodo(struct inodo *inodo, int ninodo) {
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

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, RED "Sintaxis: %s <nombre_dispositivo>\n" RESET, argv[0]);
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO) return FALLO;

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;

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

    for (int i = 0; i < 10; i++) {
        printf("%d ", inodo_actual); // Imprimimos el inodo que estamos visitando
        
        // 2. Lo traemos a RAM
        leer_inodo(inodo_actual, &inodo_aux);
        
        // 3. El siguiente paso en nuestro camino es lo que guarde su puntero
        inodo_actual = inodo_aux.punterosDirectos[0];
        
        // 4. Medida de seguridad por si llegamos al final antes de tiempo
        if (inodo_actual == UINT_MAX) {
            printf("... (FIN)");
            break;
        }
    }
    printf("\n");

    // 2. Test Reserva/Liberación Bloque
    printf("\nRESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS\n");
    int nbloque = reservar_bloque();
    bread(posSB, &SB); // Recargar SB para ver cambios
    printf("Se ha reservado el bloque fisico n%d. Bloques libres: %d\n", nbloque, SB.cantBloquesLibres);
    
    liberar_bloque(nbloque);
    bread(posSB, &SB);
    printf("Liberamos ese bloque. Bloques libres: %d\n", SB.cantBloquesLibres);

    // 3. Test Lectura de Bits (Metadatos)
    printf("\nMAPA DE BITS (Muestreo)\n");
    printf("posSB (%d) -> leer_bit(%d) = %d\n", posSB, posSB, leer_bit(posSB));
    printf("posPrimerBloqueMB (%d) -> leer_bit(%d) = %d\n", SB.posPrimerBloqueMB, SB.posPrimerBloqueMB, leer_bit(SB.posPrimerBloqueMB));
    printf("posUltimoBloqueAI (%d) -> leer_bit(%d) = %d\n", SB.posUltimoBloqueAI, SB.posUltimoBloqueAI, leer_bit(SB.posUltimoBloqueAI));
    printf("posPrimerBloqueDatos (%d) -> leer_bit(%d) = %d\n", SB.posPrimerBloqueDatos, SB.posPrimerBloqueDatos, leer_bit(SB.posPrimerBloqueDatos));
    printf("posUltimoBloqueDatos (%d) -> leer_bit(%d) = %d\n", SB.posUltimoBloqueDatos, SB.posUltimoBloqueDatos, leer_bit(SB.posUltimoBloqueDatos));

    // 4. Test Directorio Raíz
    struct inodo inodoRaiz;
    if (leer_inodo(SB.posInodoRaiz, &inodoRaiz) == EXITO) {
        mostrar_inodo(&inodoRaiz, SB.posInodoRaiz);
    } else {
        printf("Error al leer el inodo raiz.\n");
    }

    if (bumount() == FALLO) return FALLO;
    return EXITO;
}