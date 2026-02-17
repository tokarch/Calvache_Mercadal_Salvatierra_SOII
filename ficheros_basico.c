// ficheros_basico.c
// Descripción: Implementación de funciones de bajo nivel para metadatos.

#include "ficheros_basico.h"

// Calcula el tamaño en bloques del Mapa de Bits
int tamMB(unsigned int nbloques) {
    // Cada bloque del MB gestiona BLOCKSIZE * 8 bloques del disco.
    // Fórmula: (nbloques / 8) / BLOCKSIZE. Si hay resto, sumamos 1 bloque.
    int tam = (nbloques / 8) / BLOCKSIZE;
    if ((nbloques / 8) % BLOCKSIZE != 0) {
        tam++;
    }
    return tam;
}

// Calcula el tamaño en bloques del Array de Inodos
int tamAI(unsigned int ninodos) {
    // Caben (BLOCKSIZE / INODOSIZE) inodos en un bloque.
    int tam = (ninodos * INODOSIZE) / BLOCKSIZE;
    if ((ninodos * INODOSIZE) % BLOCKSIZE != 0) {
        tam++;
    }
    return tam;
}

// Inicializa el Superbloque
int initSB(unsigned int nbloques, unsigned int ninodos) {
    struct superbloque SB;

    // Posicionamiento de estructuras
    SB.posPrimerBloqueMB = posSB + tamSB;
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;
    
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;
    
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
    SB.posUltimoBloqueDatos = nbloques - 1;

    // Inicialización de inodos
    SB.posInodoRaiz = 0;
    SB.posPrimerInodoLibre = 0; // Lista enlazada empieza en 0

    // Contadores
    SB.cantBloquesLibres = nbloques; // Se actualizará en initMB
    SB.cantInodosLibres = ninodos;
    SB.totBloques = nbloques;
    SB.totInodos = ninodos;

    // Escribimos el SB en el bloque 0
    if (bwrite(posSB, &SB) == FALLO) {
        return FALLO;
    }
    return EXITO;
}

// Inicializa el Mapa de Bits
int initMB() {
    // Leemos el SB para saber las dimensiones
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;

    // Calculamos cuánto ocupan los metadatos (SB + MB + AI)
    // El primer bloque de datos libres es posPrimerBloqueDatos.
    // Por tanto, los bloques 0 a (posPrimerBloqueDatos - 1) son metadatos.
    unsigned int tamMetadatos = SB.posPrimerBloqueDatos;
    
    // Calculamos cuántos bytes y bits completos necesitamos poner a 1
    unsigned int bytesCompletos = tamMetadatos / 8;
    unsigned int bitsRestantes = tamMetadatos % 8;

    unsigned char buffer[BLOCKSIZE];
    // Inicializamos todo a 0 por defecto
    memset(buffer, 0, BLOCKSIZE); 

    // Recorremos los bloques del MB
    // (Generalmente cabe en 1, pero hacemos el bucle por si el disco es enorme)
    unsigned int bytesEscritos = 0;
    
    for (unsigned int i = SB.posPrimerBloqueMB; i <= SB.posUltimoBloqueMB; i++) {
        // Para cada byte del bloque
        for (int j = 0; j < BLOCKSIZE; j++) {
            if (bytesEscritos < bytesCompletos) {
                buffer[j] = 0xFF; // 11111111 -> Todo ocupado
                bytesEscritos++;
            } else if (bytesEscritos == bytesCompletos && bitsRestantes > 0) {
                // Último byte parcial. Ejemplo: si bitsRestantes=3 -> 11100000
                // Usamos desplazamiento de bits para crear la máscara
                unsigned char mascara = 0;
                for (int k = 0; k < bitsRestantes; k++) {
                   mascara |= (1 << (7 - k));
                }
                buffer[j] = mascara;
                bytesEscritos++; // Ya hemos "gastado" la parte de bits
                bitsRestantes = 0; // No quedan bits por marcar
            } else {
                buffer[j] = 0x00; // El resto son bloques de datos libres
            }
        }
        // Escribimos el bloque del MB modificado
        if (bwrite(i, buffer) == FALLO) return FALLO;
        
        // Reiniciamos buffer para el siguiente bloque (aunque ya será todo 0)
        memset(buffer, 0, BLOCKSIZE);
    }

    // Actualizamos la cantidad de bloques libres en el SB
    SB.cantBloquesLibres -= tamMetadatos;
    if (bwrite(posSB, &SB) == FALLO) return FALLO;

    return EXITO;
}

// Inicializa el Array de Inodos
int initAI() {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;

    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    unsigned int contInodos = SB.posPrimerInodoLibre + 1; // El siguiente al 0 es el 1
    unsigned int inodoAbsoluto = 0; // Para saber qué inodo estamos tratando realmente
    // Recorremos todos los bloques del AI
    for (unsigned int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++) {
        // Inicializamos los inodos del bloque
        for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++) {
            inodos[j].tipo = 'l'; // Libre
            
            if (contInodos < SB.totInodos) {
                inodos[j].punterosDirectos[0] = contInodos; // Apunta al siguiente
                contInodos++;
            } else {
                inodos[j].punterosDirectos[0] = UINT_MAX; // Fin de la lista
            }
            inodoAbsoluto++;
        }
        // Escribimos el bloque de inodos
        if (bwrite(i, inodos) == FALLO) return FALLO;
    }
    return EXITO;
}