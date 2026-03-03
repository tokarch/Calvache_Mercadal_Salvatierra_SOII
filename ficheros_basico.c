// ficheros_basico.c
// Implementación actualizada con lógica de traducción de bloques (Nivel 4).

#include "ficheros_basico.h"

// ==============================================================================
// FUNCIONES NIVEL 2
// ==============================================================================

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

// ==============================================================================
// FUNCIONES NIVEL 3
// ==============================================================================

/**
 * escribir_bit
 * Escribe un valor (0 o 1) en el bit correspondiente al bloque físico indicado.
 */
int escribir_bit(unsigned int nbloque, unsigned int bit) {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;

    unsigned int posbyte = nbloque / 8;
    unsigned int posbit = nbloque % 8;
    unsigned int nbloqueMB = posbyte / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    unsigned char bufferMB[BLOCKSIZE];
    if (bread(nbloqueabs, bufferMB) == FALLO) return FALLO;

    posbyte = posbyte % BLOCKSIZE; // Ajuste dentro del buffer

    unsigned char mascara = 128; // 10000000
    mascara >>= posbit; // Desplazamos el 1 a la posición correcta

    if (bit == 1) {
        bufferMB[posbyte] |= mascara; // OR para poner a 1
    } else {
        bufferMB[posbyte] &= ~mascara; // AND con NOT para poner a 0
    }

    if (bwrite(nbloqueabs, bufferMB) == FALLO) return FALLO;
    return EXITO;
}

/**
 * leer_bit
 * Lee el valor del bit correspondiente al bloque físico indicado.
 */
char leer_bit(unsigned int nbloque) {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;

    unsigned int posbyte = nbloque / 8;
    unsigned int posbit = nbloque % 8;
    unsigned int nbloqueMB = posbyte / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    unsigned char bufferMB[BLOCKSIZE];
    if (bread(nbloqueabs, bufferMB) == FALLO) return FALLO;

    posbyte = posbyte % BLOCKSIZE;

    unsigned char mascara = 128; // 10000000
    mascara >>= posbit;
    mascara &= bufferMB[posbyte]; // Filtramos el bit
    
    return (mascara >> (7 - posbit)); // Lo movemos al final para devolver 0 o 1
}

/**
 * reservar_bloque
 * Encuentra el primer bloque libre en el MB, lo marca como ocupado y devuelve su ID.
 */
int reservar_bloque() {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;

    if (SB.cantBloquesLibres == 0) return FALLO; // No queda espacio

    unsigned char bufferMB[BLOCKSIZE];
    unsigned char bufferAux[BLOCKSIZE];
    memset(bufferAux, 255, BLOCKSIZE); // Buffer lleno de 1s para comparaciones rápidas

    unsigned int nbloqueabs = SB.posPrimerBloqueMB;
    int encontrado = 0;
    unsigned int posbyte = 0;
    unsigned int posbit = 0;

    // 1. Buscamos el bloque del MB que tenga algún 0
    while (nbloqueabs <= SB.posUltimoBloqueMB && !encontrado) {
        if (bread(nbloqueabs, bufferMB) == FALLO) return FALLO;
        
        // Si memcmp devuelve 0, son iguales (todo 1s), pasamos al siguiente
        if (memcmp(bufferMB, bufferAux, BLOCKSIZE) != 0) {
            encontrado = 1;
            // 2. Buscamos el byte dentro del bloque que tenga algún 0
            posbyte = 0;
            while (posbyte < BLOCKSIZE && bufferMB[posbyte] == 255) {
                posbyte++;
            }
            
            // 3. Buscamos el bit dentro del byte que sea 0
            unsigned char mascara = 128;
            posbit = 0;
            while (bufferMB[posbyte] & mascara) { // Mientras sea 1
                bufferMB[posbyte] <<= 1; // Desplazamos para ver el siguiente
                posbit++;
            }
        } else {
            nbloqueabs++;
        }
    }

    if (!encontrado) return FALLO; // Seguridad

    // Recalculamos el nbloque físico real
    unsigned int nbloqueMB = nbloqueabs - SB.posPrimerBloqueMB;
    unsigned int nbloque = (nbloqueMB * BLOCKSIZE + posbyte) * 8 + posbit;

    // 4. Marcamos como ocupado
    if (escribir_bit(nbloque, 1) == FALLO) return FALLO;

    // 5. Decrementamos contador en SB
    SB.cantBloquesLibres--;
    if (bwrite(posSB, &SB) == FALLO) return FALLO;

    // 6. Limpiamos el bloque de datos (llenar de 0s) para evitar "basura"
    memset(bufferAux, 0, BLOCKSIZE);
    if (bwrite(nbloque, bufferAux) == FALLO) return FALLO;

    return nbloque;
}

/**
 * liberar_bloque
 * Libera un bloque marcado en el MB.
 */
int liberar_bloque(unsigned int nbloque) {
    if (escribir_bit(nbloque, 0) == FALLO) return FALLO;

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;

    SB.cantBloquesLibres++;
    if (bwrite(posSB, &SB) == FALLO) return FALLO;

    return nbloque;
}

/**
 * escribir_inodo
 * Escribe una estructura inodo en el array de inodos del disco.
 */
int escribir_inodo(unsigned int ninodo, struct inodo *inodo) {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;

    unsigned int nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueAI + nbloqueAI;
    
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    if (bread(nbloqueabs, inodos) == FALLO) return FALLO;

    // Calculamos el índice dentro del bloque
    int indice = ninodo % (BLOCKSIZE / INODOSIZE);
    inodos[indice] = *inodo;

    if (bwrite(nbloqueabs, inodos) == FALLO) return FALLO;
    return EXITO;
}

/**
 * leer_inodo
 * Lee un inodo del disco y lo guarda en la variable pasada por referencia.
 */
int leer_inodo(unsigned int ninodo, struct inodo *inodo) {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;

    unsigned int nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueAI + nbloqueAI;

    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    if (bread(nbloqueabs, inodos) == FALLO) return FALLO;

    int indice = ninodo % (BLOCKSIZE / INODOSIZE);
    *inodo = inodos[indice];

    return EXITO;
}

/**
 * reservar_inodo
 * Reserva el primer inodo libre de la lista enlazada, lo inicializa y devuelve su ID.
 */
int reservar_inodo(unsigned char tipo, unsigned char permisos) {
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;

    if (SB.cantInodosLibres == 0) return FALLO;

    // 1. Obtenemos el ID del primer inodo libre
    unsigned int posInodoReservado = SB.posPrimerInodoLibre;

    // 2. Leemos ese inodo para saber a cuál apunta (el siguiente libre)
    struct inodo inodoReservado;
    if (leer_inodo(posInodoReservado, &inodoReservado) == FALLO) return FALLO;

    // 3. Actualizamos el SB para que apunte al siguiente
    SB.posPrimerInodoLibre = inodoReservado.punterosDirectos[0];
    SB.cantInodosLibres--;
    if (bwrite(posSB, &SB) == FALLO) return FALLO;

    // 4. Inicializamos el inodo reservado con los datos nuevos
    inodoReservado.tipo = tipo;
    inodoReservado.permisos = permisos;
    inodoReservado.nlinks = 1;
    inodoReservado.tamEnBytesLog = 0;
    inodoReservado.numBloquesOcupados = 0;
    
    // Fechas actuales
    time_t fechaActual = time(NULL);
    inodoReservado.atime = fechaActual;
    inodoReservado.mtime = fechaActual;
    inodoReservado.ctime = fechaActual;
    inodoReservado.btime = fechaActual;

    // Poner punteros a 0
    memset(inodoReservado.punterosDirectos, 0, sizeof(inodoReservado.punterosDirectos));
    memset(inodoReservado.punterosIndirectos, 0, sizeof(inodoReservado.punterosIndirectos));

    // 5. Escribimos el inodo ya inicializado
    if (escribir_inodo(posInodoReservado, &inodoReservado) == FALLO) return FALLO;

    return posInodoReservado;
}

// ==============================================================================
// FUNCIONES NIVEL 4 (Direccionamiento Multinivel)
// ==============================================================================

/**
 * obtener_nRangoBL
 * Obtiene el rango de punteros (Directos, Indirectos0, 1 o 2) para un bloque lógico.
 * Actualiza *ptr con el puntero inicial correspondiente del inodo.
 */
int obtener_nRangoBL(struct inodo *inodo, unsigned int nblogico, unsigned int *ptr) {
    if (nblogico < DIRECTOS) {
        *ptr = inodo->punterosDirectos[nblogico];
        return 0; // Rango Directo
    } else if (nblogico < INDIRECTOS0) {
        *ptr = inodo->punterosIndirectos[0];
        return 1; // Indirecto Simple
    } else if (nblogico < INDIRECTOS1) {
        *ptr = inodo->punterosIndirectos[1];
        return 2; // Indirecto Doble
    } else if (nblogico < INDIRECTOS2) {
        *ptr = inodo->punterosIndirectos[2];
        return 3; // Indirecto Triple
    }

    *ptr = 0;
    fprintf(stderr, "Error: Bloque lógico %d fuera de rango\n", nblogico);
    return FALLO;
}

/**
 * obtener_indice
 * Calcula el índice dentro de un bloque de punteros para un nivel específico.
 * nivel_punteros: 3 (apunta a indirectos dobles), 2 (apunta a simples), 1 (apunta a datos).
 */
int obtener_indice(unsigned int nblogico, int nivel_punteros) {
    if (nblogico < DIRECTOS) {
        return nblogico; // Directo, no hay índice dentro de bloque
    } else if (nblogico < INDIRECTOS0) {
        return nblogico - DIRECTOS; // Indirecto 0 (nivel 1)
    } else if (nblogico < INDIRECTOS1) { // Indirecto 1
        if (nivel_punteros == 2) {
            return (nblogico - INDIRECTOS0) / NPUNTEROS;
        } else if (nivel_punteros == 1) {
            return (nblogico - INDIRECTOS0) % NPUNTEROS;
        }
    } else if (nblogico < INDIRECTOS2) { // Indirecto 2
        if (nivel_punteros == 3) {
            return (nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS);
        } else if (nivel_punteros == 2) {
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS;
        } else if (nivel_punteros == 1) {
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) % NPUNTEROS;
        }
    }
    return FALLO;
}

/**
 * traducir_bloque_inodo
 * Convierte un bloque lógico en físico. Si reservar==1, asigna espacio si no existe.
 */
int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, unsigned char reservar) {
    struct inodo inodo;
    unsigned int ptr, ptr_ant;
    int salvar_inodo = 0;
    unsigned int buffer[NPUNTEROS];
    int nRangoBL, nivel_punteros, indice;

    // Leemos el inodo del disco
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;

    // Obtenemos en qué rango cae el bloque lógico y el puntero inicial
    ptr = 0;
    nRangoBL = obtener_nRangoBL(&inodo, nblogico, &ptr);
    nivel_punteros = nRangoBL; // Empezamos por el nivel más alto

    // Iteramos bajando niveles hasta llegar a los datos (nivel 0 implícito al salir del while)
    while (nivel_punteros > 0) {
        if (ptr == 0) { // Hueco en los punteros intermedios
            if (reservar == 0) return FALLO; // Error si solo consultamos

            // Reservar bloque de punteros y poner a 0
            ptr = reservar_bloque();
            inodo.numBloquesOcupados++;
            inodo.ctime = time(NULL);
            salvar_inodo = 1; // Habrá que guardar cambios en el inodo al final
            
            // Si el bloque nuevo cuelga directamente del inodo
            if (nivel_punteros == nRangoBL) {
                inodo.punterosIndirectos[nRangoBL - 1] = ptr;
                printf("[traducir_bloque_inodo() -> inodo.punterosIndirectos[%d] = %d (reservado BF %d para punteros_nivel%d)]\n", 
                        nRangoBL - 1, ptr, ptr, nivel_punteros);
            } else { 
                // Si cuelga de otro bloque de punteros, actualizamos el buffer anterior y lo guardamos
                buffer[indice] = ptr;
                printf("[traducir_bloque_inodo() -> punteros_nivel%d [%d] = %d (reservado BF %d para punteros_nivel%d)]\n", 
                       nivel_punteros + 1, indice, ptr, ptr, nivel_punteros);
                bwrite(ptr_ant, buffer);
            }
            
            // Inicializamos el nuevo bloque de punteros a 0 en memoria y disco
            memset(buffer, 0, BLOCKSIZE); 
        } else {
            // Si el bloque ya existía, lo leemos
            bread(ptr, buffer);
        }

        // Preparamos para el siguiente nivel
        indice = obtener_indice(nblogico, nivel_punteros);
        ptr_ant = ptr;         // Guardamos el bloque actual (padre)
        ptr = buffer[indice];  // Avanzamos al siguiente puntero (hijo)
        nivel_punteros--;
    }

    // Al salir del bucle, ptr contiene la dirección del bloque de DATOS (o 0 si no existe)
    if (ptr == 0) {
        if (reservar == 0) return FALLO; // Error lectura lectura

        // Reservar bloque de DATOS
        ptr = reservar_bloque();
        inodo.numBloquesOcupados++;
        inodo.ctime = time(NULL);
        salvar_inodo = 1;

        if (nRangoBL == 0) { // Directo
            inodo.punterosDirectos[nblogico] = ptr;
            printf("[traducir_bloque_inodo() -> inodo.punterosDirectos[%d] = %d (reservado BF %d para BL %d)]\n", 
                   nblogico, ptr, ptr, nblogico);
        } else { // Indirecto (ptr_ant tiene el bloque de punteros de nivel 1)
            buffer[indice] = ptr;
            printf("[traducir_bloque_inodo() -> punteros_nivel1 [%d] = %d (reservado BF %d para BL %d)]\n", 
                   indice, ptr, ptr, nblogico);
            bwrite(ptr_ant, buffer); // Salvamos el bloque de punteros padre
        }
    }

    // Guardamos el inodo solo si hubo cambios
    if (salvar_inodo) {
        escribir_inodo(ninodo, &inodo);
    }

    return ptr; // Devolvemos el Nº de bloque físico
}