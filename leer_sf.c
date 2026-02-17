// leer_sf.c
// Descripción: Programa auxiliar para verificar la estructura del SF.

#include "ficheros_basico.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, RED "Sintaxis: %s <nombre_dispositivo>\n" RESET, argv[0]);
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO) return FALLO;

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;

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

    /* Opcional: Recorrido de inodos libres para verificar lista enlazada
    printf("\nRECORRIDO LISTA INODOS LIBRES (Primeros 10)...\n");
    struct inodo inodos[BLOCKSIZE/INODOSIZE];
    // Esto es solo un ejemplo de lectura del primer bloque del AI
    bread(SB.posPrimerBloqueAI, inodos);
    for (int i=0; i<10; i++) {
        printf("%d ", inodos[i].punterosDirectos[0]);
    }
    printf("\n");
    */

    if (bumount() == FALLO) return FALLO;
    return EXITO;
}