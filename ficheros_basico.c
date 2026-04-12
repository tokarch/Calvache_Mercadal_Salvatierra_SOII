// ficheros_basico.c
// Implementación actualizada con lógica de traducción de bloques (Nivel 4).

#include "ficheros_basico.h"

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

// Contadores para el DEBUG del Nivel 6
int cbreads = 0;
int cbwrites = 0;

// ==============================================================================
// FUNCIONES NIVEL 2
// ==============================================================================

/**
 * tamMB
 * ------
 * Función para calcular el tamaño en bloques del Mapa de Bits.
 * Parámetros:
 * - nbloques: número total de bloques del disco.
 * Retorno:
 * - Devuelve el tamaño en bloques del Mapa de Bits.
 * Llama a: -
 * Es llamada por: initSB()
 */
int tamMB(unsigned int nbloques)
{
    // Cada bloque del MB gestiona BLOCKSIZE * 8 bloques del disco.
    // Fórmula: (nbloques / 8) / BLOCKSIZE. Si hay resto, sumamos 1 bloque.
    int tam = (nbloques / 8) / BLOCKSIZE;
    if ((nbloques / 8) % BLOCKSIZE != 0)
    {
        tam++;
    }
    return tam;
}

/**
 * tamAI
 * ------
 * Función para calcular el tamaño en bloques del Array de Inodos.
 * Parámetros:
 * - ninodos: número total de inodos del disco.
 * Retorno:
 * - Devuelve el tamaño en bloques del Array de Inodos.
 * Llama a: -
 * Es llamada por: initSB()
 */
int tamAI(unsigned int ninodos)
{
    // Caben (BLOCKSIZE / INODOSIZE) inodos en un bloque.
    int tam = (ninodos * INODOSIZE) / BLOCKSIZE;
    if ((ninodos * INODOSIZE) % BLOCKSIZE != 0)
    {
        tam++;
    }
    return tam;
}

/**
 * initSB
 * ------
 * Función para inicializar el Superbloque.
 * Parámetros:
 * - nbloques: número total de bloques del disco.
 * - ninodos: número total de inodos del disco.
 * Retorno:
 * - Devuelve el descriptor del fichero o FALLO (-1) en caso de error.
 * Llama a: tamMB(), tamAI(), bwrite()
 * Es llamada por: mi_mkfs()
 */
// Inicializa el Superbloque
int initSB(unsigned int nbloques, unsigned int ninodos)
{
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
    if (bwrite(posSB, &SB) == FALLO)
    {
        return FALLO;
    }
    return EXITO;
}

/**
 * initMB
 * ------
 * Función para inicializar el Mapa de Bits.
 * Parámetros:
 * - Ninguno.
 * Retorno:
 * - Devuelve EXITO (0) o FALLO (-1) en caso de error.
 * Llama a: bread(), memset(), bwrite()
 * Es llamada por: mi_mkfs()
 */
// Inicializa el Mapa de Bits
int initMB()
{
    // Leemos el SB para saber las dimensiones
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

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

    for (unsigned int i = SB.posPrimerBloqueMB; i <= SB.posUltimoBloqueMB; i++)
    {
        // Para cada byte del bloque
        for (int j = 0; j < BLOCKSIZE; j++)
        {
            if (bytesEscritos < bytesCompletos)
            {
                buffer[j] = 0xFF; // 11111111 -> Todo ocupado
                bytesEscritos++;
            }
            else if (bytesEscritos == bytesCompletos)
            {
                // Último byte parcial. Ejemplo: si bitsRestantes=3 -> 11100000
                // Usamos desplazamiento de bits para crear la máscara
                unsigned char mascara = 0;
                if (bitsRestantes > 0)
                {
                    mascara = (unsigned char)(0xFF << (8 - bitsRestantes)); // Desplazamos y enmascaramos para quedarnos solo con los bits necesarios
                }
                buffer[j] = mascara; // Si no quedan bits restantes, la mascara sera 0
                bytesEscritos++;     // Ya hemos "gastado" la parte de bits
                bitsRestantes = 0;   // No quedan bits por marcar
            }
        }
        // Escribimos el bloque del MB modificado
        if (bwrite(i, buffer) == FALLO)
            return FALLO;

        // Reiniciamos buffer para el siguiente bloque (aunque ya será todo 0)
        memset(buffer, 0, BLOCKSIZE);
    }

    // Actualizamos la cantidad de bloques libres en el SB
    SB.cantBloquesLibres -= tamMetadatos;
    if (bwrite(posSB, &SB) == FALLO)
        return FALLO;

    return EXITO;
}

/**
 * initAI
 * ------
 * Función para inicializar el Array de Inodos.
 * Parámetros:
 * - Ninguno.
 * Retorno:
 * - Devuelve EXITO (0) o FALLO (-1) en caso de error.
 * Llama a: bread(), memset(), bwrite()
 * Es llamada por: mi_mkfs()
 */
int initAI()
{
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    memset(inodos, 0, sizeof(inodos));                    // Inicializa todo el bloque a 0 para evitar bytes indeterminados
    unsigned int contInodos = SB.posPrimerInodoLibre + 1; // El siguiente al 0 es el 1
    unsigned int inodoAbsoluto = 0;                       // Para saber qué inodo estamos tratando realmente
    // Recorremos todos los bloques del AI
    for (unsigned int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++)
    {
        // Inicializamos los inodos del bloque
        for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++)
        {
            inodos[j].tipo = 'l'; // Libre

            if (contInodos < SB.totInodos)
            {
                inodos[j].punterosDirectos[0] = contInodos; // Apunta al siguiente
                contInodos++;
            }
            else
            {
                inodos[j].punterosDirectos[0] = UINT_MAX; // Fin de la lista
                // Si hay slots sobrantes, también los marcamos como fin de lista para evitar confusiones
            }
            inodoAbsoluto++;
        }
        // Escribimos el bloque de inodos
        if (bwrite(i, inodos) == FALLO)
            return FALLO;
    }
    return EXITO;
}

// ==============================================================================
// FUNCIONES NIVEL 3
// ==============================================================================

/**
 * escribir_bit
 * ------
 * Escribe el valor indicado (0 o 1) en un bit concreto del Mapa de Bits.
 * Parámetros:
 * - nbloque: número físico de bloque que queremos marcar.
 * - bit: valor a escribir (0 o 1).
 * Retorno:
 * - Devuelve EXITO (0) o FALLO (-1) en caso de error.
 * Llama a: bread(), bwrite()
 * Es llamada por: reservar_bloque(), liberar_bloque()
 */
int escribir_bit(unsigned int nbloque, unsigned int bit)
{
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    unsigned int posbyteMB = nbloque / 8;
    unsigned int nbloqueMB = posbyteMB / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;
    unsigned int posbit = nbloque % 8;

    unsigned char bufferMB[BLOCKSIZE];
    if (bread(nbloqueabs, bufferMB) == FALLO)
        return FALLO;

    unsigned int posbyte = posbyteMB % BLOCKSIZE; // Ajuste dentro del buffer

    unsigned char mascara = 128; // 10000000
    mascara >>= posbit;          // Desplazamos el 1 a la posición correcta

    if (bit == 1)
    {
        bufferMB[posbyte] |= mascara; // OR para poner a 1
    }
    else
    {
        bufferMB[posbyte] &= ~mascara; // AND con NOT para poner a 0
    }

    if (bwrite(nbloqueabs, bufferMB) == FALLO)
        return FALLO;
    return EXITO;
}

/**
 * leer_bit
 * ------
 * Lee el valor correspondiente a un bit del Mapa de Bits.
 * Parámetros:
 * - nbloque: número físico del bloque que queremos comprobar.
 * Retorno:
 * - Devuelve el valor del bit (0 o 1), o FALLO (-1) en caso de error.
 * Llama a: bread()
 * Es llamada por: Utilidades de depuración o capas superiores.
 */
char leer_bit(unsigned int nbloque)
{
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    unsigned int posbyte = nbloque / 8;
    unsigned int posbit = nbloque % 8;
    unsigned int nbloqueMB = posbyte / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    unsigned char bufferMB[BLOCKSIZE];
    if (bread(nbloqueabs, bufferMB) == FALLO)
        return FALLO;

    // Aquí posbyte ya es el total. Necesitamos posbyte dentro del bloque (posbyte_bloque)
    // para leer del buffer, y posbyteMB (que es posbyte) para el debug.
    unsigned int posbyteMB = posbyte;
    unsigned int posbyte_bloque = posbyteMB % BLOCKSIZE;

    unsigned char mascara = 128; // 10000000
    mascara >>= posbit;
    mascara &= bufferMB[posbyte_bloque]; // Filtramos el bit

    // Imprimir mensaje de depuración (en rojo) si DEBUGN3 está activo
#if DEBUGN3
    fprintf(stderr, BLACK "[leer_bit(%u)→ posbyteMB:%u, posbyte:%u, posbit:%u, nbloqueMB:%u, nbloqueabs:%u)]\n" RESET,
            nbloque, posbyteMB, posbyte_bloque, posbit, nbloqueMB, nbloqueabs);
#endif

    return (mascara >> (7 - posbit)); // Lo movemos al final para devolver 0 o 1
}

/**
 * reservar_bloque
 * ------
 * Encuentra el primer bloque libre en el Mapa de Bits, lo marca como ocupado
 * y lo inicializa a ceros en la zona de datos.
 * Parámetros:
 * - Ninguno.
 * Retorno:
 * - Devuelve el número de bloque físico reservado, o FALLO (-1) en caso de error.
 * Llama a: bread(), memcmp(), escribir_bit(), bwrite(), memset()
 * Es llamada por: traducir_bloque_inodo()
 */
int reservar_bloque()
{
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    if (SB.cantBloquesLibres == 0)
        return FALLO; // No queda espacio

    unsigned char bufferMB[BLOCKSIZE];
    unsigned char bufferAux[BLOCKSIZE];
    memset(bufferAux, 255, BLOCKSIZE); // Buffer lleno de 1s para comparaciones rápidas

    unsigned int nbloqueabs = SB.posPrimerBloqueMB;
    int encontrado = 0;
    unsigned int posbyte = 0;
    unsigned int posbit = 0;

    // 1. Buscamos el bloque del MB que tenga algún 0
    while (nbloqueabs <= SB.posUltimoBloqueMB && !encontrado)
    {
        if (bread(nbloqueabs, bufferMB) == FALLO)
            return FALLO;

        // Si memcmp devuelve 0, son iguales (todo 1s), pasamos al siguiente
        if (memcmp(bufferMB, bufferAux, BLOCKSIZE) != 0)
        {
            encontrado = 1;
            // 2. Buscamos el byte dentro del bloque que tenga algún 0
            posbyte = 0;
            while (posbyte < BLOCKSIZE && bufferMB[posbyte] == 255)
            {
                posbyte++;
            }

            // 3. Buscamos el bit dentro del byte que sea 0
            unsigned char mascara = 128;
            posbit = 0;
            while (bufferMB[posbyte] & mascara)
            {                            // Mientras sea 1
                bufferMB[posbyte] <<= 1; // Desplazamos para ver el siguiente
                posbit++;
            }
        }
        else
        {
            nbloqueabs++;
        }
    }

    if (!encontrado)
        return FALLO; // Seguridad

    // Recalculamos el nbloque físico real
    unsigned int nbloqueMB = nbloqueabs - SB.posPrimerBloqueMB;
    unsigned int nbloque = (nbloqueMB * BLOCKSIZE + posbyte) * 8 + posbit;

    // 4. Marcamos como ocupado
    if (escribir_bit(nbloque, 1) == FALLO)
        return FALLO;

    // 5. Decrementamos contador en SB
    SB.cantBloquesLibres--;
    if (bwrite(posSB, &SB) == FALLO)
        return FALLO;

    // 6. Limpiamos el bloque de datos (llenar de 0s) para evitar "basura"
    memset(bufferAux, 0, BLOCKSIZE);
    if (bwrite(nbloque, bufferAux) == FALLO)
        return FALLO;

    return nbloque;
}

/**
 * liberar_bloque
 * ------
 * Libera un bloque determinado en el Mapa de Bits y actualiza el contador
 * en el Superbloque.
 * Parámetros:
 * - nbloque: número físico del bloque a liberar.
 * Retorno:
 * - Devuelve el número de bloque liberado, o FALLO (-1) en caso de error.
 * Llama a: escribir_bit(), bread(), bwrite()
 * Es llamada por: liberar_directos(), liberar_indirectos_recursivo(), liberar_bloques_inodo()
 */
int liberar_bloque(unsigned int nbloque)
{
    if (escribir_bit(nbloque, 0) == FALLO)
        return FALLO;

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    SB.cantBloquesLibres++;
    if (bwrite(posSB, &SB) == FALLO)
        return FALLO;

    return nbloque;
}

/**
 * escribir_inodo
 * ------
 * Escribe el contenido de una variable de tipo inodo en el Array de Inodos 
 * en disco.
 * Parámetros:
 * - ninodo: número de inodo lógico que queremos escribir.
 * - inodo: puntero a la estructura inodo con los datos.
 * Retorno:
 * - Devuelve EXITO (0) o FALLO (-1) en caso de error.
 * Llama a: bread(), bwrite()
 * Es llamada por: reservar_inodo(), traducir_bloque_inodo(), liberar_inodo()
 */
int escribir_inodo(unsigned int ninodo, struct inodo *inodo)
{
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    unsigned int nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueAI + nbloqueAI;

    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    if (bread(nbloqueabs, inodos) == FALLO)
        return FALLO;

    // Calculamos el índice dentro del bloque
    int indice = ninodo % (BLOCKSIZE / INODOSIZE);
    inodos[indice] = *inodo;

    if (bwrite(nbloqueabs, inodos) == FALLO)
        return FALLO;
    return EXITO;
}

/**
 * leer_inodo
 * ------
 * Lee un inodo del Array de Inodos en disco y lo vuelca en una variable 
 * pasada por referencia.
 * Parámetros:
 * - ninodo: número de inodo lógico que queremos leer.
 * - inodo: puntero a la estructura donde volcar los datos.
 * Retorno:
 * - Devuelve EXITO (0) o FALLO (-1) en caso de error.
 * Llama a: bread()
 * Es llamada por: reservar_inodo(), traducir_bloque_inodo(), liberar_inodo()
 */
int leer_inodo(unsigned int ninodo, struct inodo *inodo)
{
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    unsigned int nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueAI + nbloqueAI;

    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    if (bread(nbloqueabs, inodos) == FALLO)
        return FALLO;

    int indice = ninodo % (BLOCKSIZE / INODOSIZE);
    *inodo = inodos[indice];

    return EXITO;
}

/**
 * reservar_inodo
 * ------
 * Encuentra el primer inodo libre del array de inodos, lo reserva,
 * lo inicializa con los parámetros indicados y actualiza la lista enlazada.
 * Parámetros:
 * - tipo: carácter indicando el tipo de inodo ('d', 'f' o 'l').
 * - permisos: permisos a asignar.
 * Retorno:
 * - Devuelve el número de inodo reservado, o FALLO (-1) en caso de error.
 * Llama a: bread(), leer_inodo(), bwrite(), memset(), escribir_inodo()
 * Es llamada por: Funciones de creación (mi_creat) de capas superiores.
 */
int reservar_inodo(unsigned char tipo, unsigned char permisos)
{
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    if (SB.cantInodosLibres == 0) {
        fprintf(stderr, RED "Error: No hay inodos libres en el dispositivo.\n" RESET);
        return FALLO;
    }

    // 1. Obtenemos el ID del primer inodo libre
    unsigned int posInodoReservado = SB.posPrimerInodoLibre;

    // 2. Leemos ese inodo para saber a cuál apunta (el siguiente libre)
    struct inodo inodoReservado;
    if (leer_inodo(posInodoReservado, &inodoReservado) == FALLO)
        return FALLO;

    // 3. Actualizamos el SB para que apunte al siguiente
    SB.posPrimerInodoLibre = inodoReservado.punterosDirectos[0];
    SB.cantInodosLibres--;
    if (bwrite(posSB, &SB) == FALLO)
        return FALLO;

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
    if (escribir_inodo(posInodoReservado, &inodoReservado) == FALLO)
        return FALLO;

    return posInodoReservado;
}

// ==============================================================================
// FUNCIONES NIVEL 4 (Direccionamiento Multinivel)
// ==============================================================================

/**
 * obtener_nRangoBL
 * ------
 * Obtiene el rango de punteros (Directos, Indirectos0, 1 o 2) en el que se 
 * sitúa un bloque lógico concreto, y el primer puntero asociado.
 * Parámetros:
 * - inodo: puntero al inodo que estamos explorando.
 * - nblogico: número del bloque lógico.
 * - ptr: puntero pasado por referencia donde se guarda la dirección de bloque.
 * Retorno:
 * - Devuelve el nivel del rango (0, 1, 2, 3), o FALLO (-1) en caso de error.
 * Llama a: -
 * Es llamada por: traducir_bloque_inodo()
 */
int obtener_nRangoBL(struct inodo *inodo, unsigned int nblogico, unsigned int *ptr)
{
    if (nblogico < DIRECTOS)
    {
        *ptr = inodo->punterosDirectos[nblogico];
        return 0; // Rango Directo
    }
    else if (nblogico < INDIRECTOS0)
    {
        *ptr = inodo->punterosIndirectos[0];
        return 1; // Indirecto Simple
    }
    else if (nblogico < INDIRECTOS1)
    {
        *ptr = inodo->punterosIndirectos[1];
        return 2; // Indirecto Doble
    }
    else if (nblogico < INDIRECTOS2)
    {
        *ptr = inodo->punterosIndirectos[2];
        return 3; // Indirecto Triple
    }

    *ptr = 0;
    fprintf(stderr, RED "Error: Bloque lógico %d fuera de rango\n" RESET, nblogico);
    return FALLO;
}

/**
 * obtener_indice
 * ------
 * Calcula el índice dentro de un bloque de punteros para un bloque lógico y
 * nivel de indirección concretos.
 * Parámetros:
 * - nblogico: número del bloque lógico a indexar.
 * - nivel_punteros: profundidad a la que nos encontramos (1, 2, 3).
 * Retorno:
 * - Devuelve el índice calculado, o FALLO (-1) en caso de error.
 * Llama a: -
 * Es llamada por: traducir_bloque_inodo()
 */
int obtener_indice(unsigned int nblogico, int nivel_punteros)
{
    if (nblogico < DIRECTOS)
    {
        return nblogico; // Directo, no hay índice dentro de bloque
    }
    else if (nblogico < INDIRECTOS0)
    {
        return nblogico - DIRECTOS; // Indirecto 0 (nivel 1)
    }
    else if (nblogico < INDIRECTOS1)
    { // Indirecto 1
        if (nivel_punteros == 2)
        {
            return (nblogico - INDIRECTOS0) / NPUNTEROS;
        }
        else if (nivel_punteros == 1)
        {
            return (nblogico - INDIRECTOS0) % NPUNTEROS;
        }
    }
    else if (nblogico < INDIRECTOS2)
    { // Indirecto 2
        if (nivel_punteros == 3)
        {
            return (nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS);
        }
        else if (nivel_punteros == 2)
        {
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS;
        }
        else if (nivel_punteros == 1)
        {
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) % NPUNTEROS;
        }
    }
    return FALLO;
}

/**
 * traducir_bloque_inodo
 * ------
 * Obtiene el número de bloque físico correspondiente a un bloque lógico 
 * determinado del inodo. Crea el camino de bloques de punteros si no existe.
 * Parámetros:
 * - ninodo: inodo que posee el bloque lógico.
 * - nblogico: bloque lógico que queremos traducir.
 * - reservar: valor flag (0: solo lectura, 1: reserva bloques si no existen).
 * Retorno:
 * - Devuelve el número de bloque físico correspondiente, o FALLO (-1).
 * Llama a: leer_inodo(), obtener_nRangoBL(), reservar_bloque(), bwrite(), memset(), bread(), obtener_indice(), escribir_inodo()
 * Es llamada por: Funciones de lectura/escritura (mi_read_f, mi_write_f).
 */
int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, unsigned char reservar)
{
    struct inodo inodo;
    unsigned int ptr, ptr_ant;
    int salvar_inodo = 0;
    unsigned int buffer[NPUNTEROS];
    int nRangoBL, nivel_punteros, indice;

    // Leemos el inodo del disco
    if (leer_inodo(ninodo, &inodo) == FALLO)
        return FALLO;

    // Obtenemos en qué rango cae el bloque lógico y el puntero inicial
    ptr = 0;
    nRangoBL = obtener_nRangoBL(&inodo, nblogico, &ptr);
    nivel_punteros = nRangoBL; // Empezamos por el nivel más alto

    // Iteramos bajando niveles hasta llegar a los datos (nivel 0 implícito al salir del while)
    while (nivel_punteros > 0)
    {
        if (ptr == 0)
        { // Hueco en los punteros intermedios
            if (reservar == 0)
                return FALLO; // Error si solo consultamos

            // Reservar bloque de punteros y poner a 0
            ptr = reservar_bloque();
            inodo.numBloquesOcupados++;
            inodo.ctime = time(NULL);
            salvar_inodo = 1; // Habrá que guardar cambios en el inodo al final

            // Si el bloque nuevo cuelga directamente del inodo
            if (nivel_punteros == nRangoBL)
            {
                inodo.punterosIndirectos[nRangoBL - 1] = ptr;
                #if DEBUGN4 || DEBUGN5 || DEBUGN6
                fprintf(stderr, BLACK "[traducir_bloque_inodo() -> inodo.punterosIndirectos[%d] = %d (reservado BF %d para punteros_nivel%d)]\n RESET",
                       nRangoBL - 1, ptr, ptr, nivel_punteros);
                #endif
            }
            else
            {
                // Si cuelga de otro bloque de punteros, actualizamos el buffer anterior y lo guardamos
                buffer[indice] = ptr;
                #if DEBUGN4 || DEBUGN5 || DEBUGN6
                fprintf(stderr, BLACK "[traducir_bloque_inodo() -> punteros_nivel%d [%d] = %d (reservado BF %d para punteros_nivel%d)]\n RESET",
                       nivel_punteros + 1, indice, ptr, ptr, nivel_punteros);
                #endif
                bwrite(ptr_ant, buffer);
            }

            // Inicializamos el nuevo bloque de punteros a 0 en memoria y disco
            memset(buffer, 0, BLOCKSIZE);
        }
        else
        {
            // Si el bloque ya existía, lo leemos
            bread(ptr, buffer);
        }

        // Preparamos para el siguiente nivel
        indice = obtener_indice(nblogico, nivel_punteros);
        ptr_ant = ptr;        // Guardamos el bloque actual (padre)
        ptr = buffer[indice]; // Avanzamos al siguiente puntero (hijo)
        nivel_punteros--;
    }

    // Al salir del bucle, ptr contiene la dirección del bloque de DATOS (o 0 si no existe)
    if (ptr == 0)
    {
        if (reservar == 0)
            return FALLO; // Error lectura lectura

        // Reservar bloque de DATOS
        ptr = reservar_bloque();
        inodo.numBloquesOcupados++;
        inodo.ctime = time(NULL);
        salvar_inodo = 1;

        if (nRangoBL == 0)
        { // Directo
            inodo.punterosDirectos[nblogico] = ptr;
            #if DEBUGN4 || DEBUGN5 || DEBUGN6
            fprintf(stderr, BLACK "[traducir_bloque_inodo() -> inodo.punterosDirectos[%d] = %d (reservado BF %d para BL %d)]\n" RESET,
                   nblogico, ptr, ptr, nblogico);
            #endif
        }
        else
        { // Indirecto (ptr_ant tiene el bloque de punteros de nivel 1)
            buffer[indice] = ptr;
            #if DEBUGN4 || DEBUGN5 || DEBUGN6
            fprintf(stderr, BLACK "[traducir_bloque_inodo() -> punteros_nivel1 [%d] = %d (reservado BF %d para BL %d)]\n" RESET,
                   indice, ptr, ptr, nblogico);
            #endif
            bwrite(ptr_ant, buffer); // Salvamos el bloque de punteros padre
        }
    }

    // Guardamos el inodo solo si hubo cambios
    if (salvar_inodo)
    {
        escribir_inodo(ninodo, &inodo);
    }

    return ptr; // Devolvemos el Nº de bloque físico
}

// ==============================================================================
// FUNCIONES AUXILIARES NIVEL 6 (Privadas para este fichero)
// ==============================================================================

/**
 * liberar_directos
 * ------
 * Recorre y libera los bloques de datos apuntados por los punteros directos.
 * Parámetros:
 * - nBL: puntero al contador global del bloque lógico actual.
 * - ultimoBL: tope máximo del fichero.
 * - inodo: puntero al inodo bajo modificación.
 * - eof: puntero al flag de fin de archivo.
 * Retorno:
 * - Devuelve la cantidad de bloques físicos liberados.
 * Llama a: liberar_bloque()
 * Es llamada por: liberar_bloques_inodo()
 */
int liberar_directos(unsigned int *nBL, unsigned int ultimoBL, struct inodo *inodo, int *eof)
{
    int liberados = 0;

    // Recorremos los punteros directos
    for (int i = *nBL; i < DIRECTOS; i++)
    {
        if (*nBL > ultimoBL)
        {
            *eof = 1;
            break;
        }

        if (inodo->punterosDirectos[i] != 0)
        {
            liberar_bloque(inodo->punterosDirectos[i]);
            inodo->punterosDirectos[i] = 0;
            liberados++;
        }
        (*nBL)++;
    }
    return liberados;
}

/**
 * liberar_indirectos_recursivo
 * ------
 * Recorre y libera recursivamente los bloques de datos y punteros apuntados 
 * en las zonas indirectas.
 * Parámetros:
 * - nBL: puntero al contador global del bloque lógico actual.
 * - ultimoBL: tope máximo de bloques lógicos del inodo.
 * - puntero_bloque: puntero al bloque (de datos o de punteros) a tratar.
 * - nivel_punteros: profundidad en la jerarquía (1, 2, 3).
 * - eof: puntero al flag de fin de archivo.
 * Retorno:
 * - Devuelve el total de bloques (datos e índice) liberados en este nivel.
 * Llama a: bread(), liberar_bloque(), liberar_indirectos_recursivo(), memset(), memcmp(), bwrite()
 * Es llamada por: liberar_bloques_inodo(), y a sí misma.
 */
int liberar_indirectos_recursivo(unsigned int *nBL, unsigned int primerBL, unsigned int ultimoBL,
                                 unsigned int *puntero_bloque, int nivel_punteros, int *eof)
{

    unsigned int bloque_punteros[NPUNTEROS];
    unsigned char buffer_ceros[BLOCKSIZE];
    int liberados = 0;
    int modificado = 0; // Para saber si hay que escribir en disco

    // Calculamos cuántos bloques lógicos abarca este nivel de punteros
    unsigned int saltos = 1;
    for (int k = 0; k < nivel_punteros; k++) {
        saltos *= NPUNTEROS;
    }

    // Entramos aquí si el bloque está vacío (0) O si todo el rango que abarca
    // está estrictamente ANTES de primerBL (por tanto, no hay que borrarlo ni leerlo)
    if (*puntero_bloque == 0 || (*nBL + saltos <= primerBL))
    {
        #if DEBUGN6
        // Solo imprimimos el salto si el puntero era 0 (para imitar el PDF)
        if (*puntero_bloque == 0 && saltos > 256) {
            fprintf(stderr, BLACK "[liberar_bloques_inodo()→ saltamos del BL %d al %d]\n" RESET, *nBL, *nBL + saltos - 1);
        }
        #endif

        *nBL += saltos; // Sumamos al contador global los bloques que nos hemos saltado

        // Ajuste si nos hemos pasado del último bloque lógico
        if (*nBL > ultimoBL) {
            *eof = 1;
        }
        return 0; // SALIMOS AQUÍ: Nos ahorramos el bread() y el bucle interno
    }

    // LECTURA DEL BLOQUE
    // Si llegamos a esta línea, significa que el bloque existe y además contiene 
    // punteros que sí debemos borrar (porque entran dentro del rango a partir de primerBL)
    if (bread(*puntero_bloque, bloque_punteros) == FALLO) {
        return FALLO;
    }

    cbreads++; // Contamos la lectura del bloque de punteros
    
    // Recorremos cada puntero del bloque
    for (int i = 0; i < NPUNTEROS; i++)
    {
        if (*eof)
            break; // Si ya hemos llegado al final del fichero, paramos

        // Caso base: estamos en nivel 1 (apunta a DATOS)
        if (nivel_punteros == 1)
        {
            if (*nBL <= ultimoBL)
            {
                if (*nBL >= primerBL && bloque_punteros[i] != 0)
                {
                    liberar_bloque(bloque_punteros[i]);
                    #if DEBUGN6
                    fprintf(stderr, BLACK "[liberar_bloques_inodo() liberado BF %d de datos correspondiente al BL %d]\n" RESET, bloque_punteros[i], *nBL);
                    #endif
                    bloque_punteros[i] = 0;
                    liberados++;
                    modificado = 1;
                }
            }
            else
            {
                *eof = 1;
            }
            (*nBL)++;
        }
        // Caso recursivo: nivel > 1 (apunta a otros bloques de punteros)
        else
        {
            liberados += liberar_indirectos_recursivo(nBL, primerBL, ultimoBL, &bloque_punteros[i], nivel_punteros - 1, eof);
            if (liberados > 0 || bloque_punteros[i] == 0)
                modificado = 1; // Si hubo cambios abajo, marcamos
        }
    }

    // Comprobamos si el bloque de punteros ha quedado vacío (todo ceros)
    memset(buffer_ceros, 0, BLOCKSIZE);
    if (memcmp(bloque_punteros, buffer_ceros, BLOCKSIZE) == 0)
    {
        // Está vacío: liberamos el bloque de punteros y ponemos el puntero padre a 0
        liberar_bloque(*puntero_bloque);
        #if DEBUGN6
        fprintf(stderr, BLACK "[liberar_bloques_inodo() liberado BF %d de punteros_nivel%d correspondiente al BL %d]\n" RESET, *puntero_bloque, nivel_punteros, *nBL);
        #endif
        *puntero_bloque = 0;
        liberados++; // Contamos el propio bloque de punteros liberado
    }
    else
    {
        // No está vacío pero ha cambiado: lo escribimos en disco
        if (modificado)
        {
            if (bwrite(*puntero_bloque, bloque_punteros) == FALLO)
                return FALLO;
        
            fprintf(stderr, BLACK "[liberar_bloques_inodo() -> salvado BF %d de punteros_nivel%d correspondiente al BL %d]\n" RESET, *puntero_bloque, nivel_punteros, *nBL);
            cbwrites++; // Contamos la escritura del bloque de punteros
        }
    }

    return liberados;
}

// ==============================================================================
// FUNCIONES PÚBLICAS NIVEL 6
// ==============================================================================

/**
 * liberar_bloques_inodo
 * ------
 * Libera todos los bloques físicos de datos y punteros que pertenecen a un inodo
 * a partir de un bloque lógico dado.
 * Parámetros:
 * - primerBL: número del primer bloque lógico a partir del cual borrar.
 * - inodo: puntero al inodo sobre el que operar.
 * Retorno:
 * - Devuelve la cantidad total de bloques físicos liberados.
 * Llama a: liberar_bloque(), liberar_indirectos_recursivo()
 * Es llamada por: liberar_inodo(), mi_truncar_f()
 */
int liberar_bloques_inodo(unsigned int primerBL, struct inodo *inodo)
{
    unsigned int nBL = 0;
    unsigned int ultimoBL;
    int liberados = 0;
    int eof = 0;

    // Reiniciamos los contadores globales para esta operación
    cbreads = 0;
    cbwrites = 0;

    // Calculamos el último bloque lógico ocupado
    if (inodo->tamEnBytesLog == 0)
        return 0;

    if (inodo->tamEnBytesLog % BLOCKSIZE == 0)
    {
        ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE - 1;
    }
    else
    {
        ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE;
    }

    #if DEBUGN6
    fprintf(stderr, "[liberar_bloques_inodo()→ primer BL: %d, último BL: %d]\n", primerBL, ultimoBL);
    #endif

    // 1. Avanzamos nBL hasta primerBL (optimizacion simple)
    // En una implementación real más compleja calcularíamos índices exactos,
    // aquí simulamos el avance rápido si nBL < primerBL en las funciones.
    // Para simplificar según el PDF (Versión 2): iteramos nBL.
    // Ajuste: si primerBL > 0, deberíamos saltar lógicamente.
    // Por simplicidad de la implementación recursiva propuesta:
    // Asumiremos que las funciones auxiliares chequean si nBL >= primerBL antes de liberar.

    // NOTA: Para implementar la lógica de "saltar" hasta primerBL correctamente,
    // deberíamos pasar primerBL a las funciones auxiliares.
    // Voy a hacer un ajuste rápido: modifico las auxiliares para que acepten primerBL si fuera necesario,
    // o simplemente iteramos desde nBL=0 y solo liberamos si nBL >= primerBL.
    // Dado que el PDF lo hace iterativo, vamos a usar nBL como contador global.

    // Corrección para eficiencia: Si nBL < primerBL, no liberamos, solo incrementamos contador.
    // En las funciones auxiliares he añadido chequeos implícitos.
    // (Ver abajo la versión mejorada de liberar_directos e indirectos).

    // --- LIBERAR DIRECTOS ---
    // Ajustamos nBL al inicio
    nBL = 0;

    // Tratamiento punteros directos
    for (int i = 0; i < DIRECTOS; i++)
    {
        if (nBL >= primerBL && nBL <= ultimoBL)
        {
            if (inodo->punterosDirectos[i] != 0)
            {
                liberar_bloque(inodo->punterosDirectos[i]);
                #if DEBUGN6
                fprintf(stderr, BLACK "[liberar_bloques_inodo() liberado BF %d de datos correspondiente al BL %d]\n" RESET, inodo->punterosDirectos[i], nBL);
                #endif
                inodo->punterosDirectos[i] = 0;
                liberados++;
            }
        }
        nBL++;
    }

    // --- LIBERAR INDIRECTOS ---

    // Indirectos 0 (Simple) - Nivel 1
    if (nBL <= ultimoBL)
    {
        liberados += liberar_indirectos_recursivo(&nBL, primerBL, ultimoBL, &inodo->punterosIndirectos[0], 1, &eof);
    }

    // Indirectos 1 (Doble) - Nivel 2
    if (!eof && nBL <= ultimoBL)
    {
        liberados += liberar_indirectos_recursivo(&nBL, primerBL, ultimoBL, &inodo->punterosIndirectos[1], 2, &eof);
    }

    // Indirectos 2 (Triple) - Nivel 3
    if (!eof && nBL <= ultimoBL)
    {
        liberados += liberar_indirectos_recursivo(&nBL, primerBL, ultimoBL, &inodo->punterosIndirectos[2], 3, &eof);
    }

    #if DEBUGN6
        fprintf(stderr, "[liberar_bloques_inodo()→ total bloques liberados: %d, total_breads: %d, total_bwrites: %d]\n", 
            liberados, cbreads, cbwrites);
    #endif

    return liberados;
}

/**
 * liberar_inodo
 * ------
 * Libera un inodo, liberando todo su contenido asociado (bloques lógicos)
 * y actualizando la cabecera de inodos libres del superbloque.
 * Parámetros:
 * - ninodo: número del inodo a liberar.
 * Retorno:
 * - Devuelve el número del inodo liberado, o FALLO (-1) en caso de error.
 * Llama a: leer_inodo(), liberar_bloques_inodo(), bread(), escribir_inodo(), bwrite()
 * Es llamada por: Funciones de borrado del sistema (mi_unlink).
 */
int liberar_inodo(unsigned int ninodo)
{
    struct inodo inodo;
    struct superbloque SB;

    // 1. Leer inodo
    if (leer_inodo(ninodo, &inodo) == FALLO)
        return FALLO;

    // 2. Liberar todos sus bloques
    int bloques_liberados = liberar_bloques_inodo(0, &inodo);
    inodo.numBloquesOcupados -= bloques_liberados;

    // Si ha ido bien, debería ser 0 (o quedarán los de metadatos si no los contamos igual)
    // Forzamos a 0 por seguridad y tipo libre
    inodo.tipo = 'l';
    inodo.tamEnBytesLog = 0;
    inodo.numBloquesOcupados = 0;

    // 3. Actualizar lista de inodos libres (Pila/Lista enlazada)
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    // El inodo liberado apunta al que antes era el primero
    inodo.punterosDirectos[0] = SB.posPrimerInodoLibre;

    // El superbloque ahora apunta a este inodo liberado como el primero
    SB.posPrimerInodoLibre = ninodo;
    SB.cantInodosLibres++;

    // Actualizamos fechas
    time_t t = time(NULL);
    inodo.ctime = t;
    inodo.mtime = t; // Opcional, pero limpio

    // 4. Escribir cambios
    if (escribir_inodo(ninodo, &inodo) == FALLO)
        return FALLO;
    if (bwrite(posSB, &SB) == FALLO)
        return FALLO;

    #if DEBUGN6
    fprintf(stderr, "[liberar_inodo()→ Tras liberar inodo: primerInodoLibre=%d]\n", SB.posPrimerInodoLibre);
    #endif

    return ninodo;
}