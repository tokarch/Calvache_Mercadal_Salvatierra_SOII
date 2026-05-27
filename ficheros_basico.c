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
    // El Mapa de Bits usa 1 bit para representar el estado de 1 bloque físico.
    // Primero calculamos cuántos bytes necesitamos: (nbloques / 8).
    // Luego dividimos esos bytes entre BLOCKSIZE para saber cuántos bloques ocupará el MB.
    int tam = (nbloques / 8) / BLOCKSIZE;

    // Si la división anterior no es exacta, significa que 
    // nos sobran bits que no caben en 'tam' bloques, así que necesitamos sumar 1 bloque adicional.
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
    // Cada inodo ocupa INODOSIZE (128 bytes). Multiplicamos el total de inodos 
    // por 128 para obtener los bytes totales, y dividimos entre BLOCKSIZE (1024) para sacar los bloques.
    // Es lo mismo que decir que caben 8 inodos por bloque (1024 / 128 = 8).
    int tam = (ninodos * INODOSIZE) / BLOCKSIZE;

    // Igual que en tamMB, comprobamos si hay resto para añadir un bloque extra si fuera necesario.
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

    // Ubicamos las zonas secuencialmente.
    // El SB siempre está en el bloque 0 (posSB). El MB empieza justo después (bloque 1).
    SB.posPrimerBloqueMB = posSB + tamSB;
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;

    // El Array de Inodos va después del MB
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;

    // La Zona de Datos empieza al terminar los metadatos y llega hasta el final del disco
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
    SB.posUltimoBloqueDatos = nbloques - 1;

    // Inicialización de inodos
    SB.posInodoRaiz = 0;
    SB.posPrimerInodoLibre = 0; // Lista enlazada empieza en 0

    // Inicialmente ponemos TODOS los bloques y TODOS los inodos como libres
    // La función initMB() se encargará luego de restar los bloques que ocupan los metadatos
    SB.cantBloquesLibres = nbloques;
    SB.cantInodosLibres = ninodos;
    SB.totBloques = nbloques;
    SB.totInodos = ninodos;

    // Escribimos el SB
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
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
        return FALLO;

    // Cantidad de bloques que ocupan los metadatos (SB + MB + AI)
    unsigned int tamMetadatos = SB.posPrimerBloqueDatos;

    // Convertimos esa cantidad a bytes completos y bits sueltos
    unsigned int bytesCompletos = tamMetadatos / 8;
    unsigned int bitsRestantes = tamMetadatos % 8;

    unsigned char buffer[BLOCKSIZE];
    unsigned int bytesEscritos = 0;

    // Iteramos entre los bloques que ocupa el MB
    for (unsigned int i = SB.posPrimerBloqueMB; i <= SB.posUltimoBloqueMB; i++)
    {
        // Limpiamos el buffer poniéndolo todo a 0 (libres)
        memset(buffer, 0, BLOCKSIZE);

        // Entramos si aún no hemos escrito todos los bytes completos que ocupan los metadatos
        if (bytesEscritos < bytesCompletos)
        {
            unsigned int bytesRestantesBloque = bytesCompletos - bytesEscritos;

            // Entramos si la cantidad de bytes por marcar supera a la de un bloque
            // COMENTARIO DE JOSÉ DAVID: OPTIMIZACIÓN CON MEMSET.
            // En lugar de iterar byte a byte, usamos memset para llenar de golpe con 255 (0xFF o 1s binarios)
            // los bytes que necesitamos marcar como ocupados en este bloque físico.
            if (bytesRestantesBloque >= BLOCKSIZE)
            {
                // El bloque del MB entero debe marcarse como ocupado
                // 255 en decimal es 11111111 en binario, así que cada byte se llena con 1s
                memset(buffer, 255, BLOCKSIZE);
                bytesEscritos += BLOCKSIZE;
            }
            else
            {
                // Solo una parte del bloque del MB se marca como ocupada
                memset(buffer, 255, bytesRestantesBloque);
                bytesEscritos += bytesRestantesBloque;

                // Si sobran bits, los metemos en el byte inmediatamente posterior
                if (bitsRestantes > 0)
                {
                    buffer[bytesRestantesBloque] = (unsigned char)(0xFF << (8 - bitsRestantes));
                    bitsRestantes = 0;
                }
            }
        }
        // Si ya hemos escrito todos los bytes completos, pero aún quedan bits sueltos, los marcamos en el siguiente byte
        else if (bitsRestantes > 0)
        {
            // CASO EXTREMO:
            // Si los bytes completos cuadraron exactamente al final del bloque anterior,
            // los bits restantes deben colocarse en el primer byte del bloque actual
            buffer[0] = (unsigned char)(0xFF << (8 - bitsRestantes));
            bitsRestantes = 0;
        }

        // Escribimos el bloque del MB modificado
        if (bwrite(i, buffer) == FALLO)
            return FALLO;
    }

    // Restamos los metadatos de los bloques libres en el SB y lo guardamos
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

    // Creamos un buffer de memoria que almacena un array de estructuras inodo
    // Su tamaño exacto es BLOCKSIZE / INODOSIZE
    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    memset(inodos, 0, sizeof(inodos));                    // Inicializa todo el bloque a 0 para evitar bytes indeterminados
    unsigned int contInodos = SB.posPrimerInodoLibre + 1; // Número del siguiente inodo libre en la lista enlazada a crear (el siguiente al 0 es el 1)
    unsigned int inodoAbsoluto = 0;                       // Para saber qué inodo estamos tratando realmente

    // Recorremos todos los bloques del AI
    for (unsigned int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++)
    {
        // Inicializamos los inodos del bloque
        for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++)
        {
            inodos[j].tipo = 'l'; // Libre

            // Si aún no estamos en el último inodo
            if (contInodos < SB.totInodos)
            {
                // Enlazamos la lista de inodos libres usando punterosDirectos[0]
                inodos[j].punterosDirectos[0] = contInodos; // Apunta al siguiente
                contInodos++;
            }
            else
            {
                // El último inodo de todos debe apuntar a "Null".
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

    // 1 byte = 8 bits. Dividimos nbloque entre 8 para saber en qué byte absoluto del MB está
    unsigned int posbyteMB = nbloque / 8;
    // Dividimos el byte absoluto entre el tamaño de bloque para saber qué bloque del MB hay que leer
    unsigned int nbloqueMB = posbyteMB / BLOCKSIZE;
    // Sumamos el bloque relativo del MB al bloque donde empieza el MB para tener la ruta física absoluta
    unsigned int nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;
    // El resto de la división entre 8 nos dice exactamente qué bit dentro del byte es el nuestro (0 a 7)
    unsigned int posbit = nbloque % 8;

    unsigned char bufferMB[BLOCKSIZE];
    if (bread(nbloqueabs, bufferMB) == FALLO)
        return FALLO;

    // posbyteMB es global al MB. El módulo nos da la posición exacta DENTRO del buffer que hemos leído
    unsigned int posbyte = posbyteMB % BLOCKSIZE;

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

    unsigned int posbyteMB = posbyte;
    unsigned int posbyte_bloque = posbyteMB % BLOCKSIZE;

    unsigned char mascara = 128; // 10000000
    mascara >>= posbit;

    // Aplicamos un AND. Esto pone a 0 todos los bits que no nos interesan
    // Si nuestro bit original era 1, la máscara se queda igual. Si era 0, la máscara entera pasa a ser 00000000
    mascara &= bufferMB[posbyte_bloque];

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
    mi_waitSem(); // <--- INICIO SC

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        mi_signalSem(); // Aseguramos liberar el semáforo incluso en caso de error
        return FALLO;
    }

    if (SB.cantBloquesLibres == 0) {
        mi_signalSem(); // Liberamos el semáforo antes de salir
        return FALLO;
    }

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
        if (bread(nbloqueabs, bufferMB) == FALLO) {
            mi_signalSem(); // Liberamos el semáforo antes de salir
            return FALLO;
        }

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
                bufferMB[posbyte] <<= 1; // Desplazamos hacia la izquierda para ver el siguiente
                posbit++;
            }
        }
        else
        {
            nbloqueabs++;
        }
    }

    if (!encontrado) {
        mi_signalSem();
        return FALLO; // Seguridad
    }

    // Teniendo el bloque, byte y bit, calculamos el número lógico del bloque físico
    unsigned int nbloqueMB = nbloqueabs - SB.posPrimerBloqueMB;
    unsigned int nbloque = (nbloqueMB * BLOCKSIZE + posbyte) * 8 + posbit;

    // 4. Marcamos como ocupado
    if (escribir_bit(nbloque, 1) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    // 5. Decrementamos contador en SB
    SB.cantBloquesLibres--;
    if (bwrite(posSB, &SB) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    // 6. Limpiamos el bloque de datos (llenar de 0s) para evitar "basura"
    memset(bufferAux, 0, BLOCKSIZE);
    if (bwrite(nbloque, bufferAux) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    mi_signalSem(); // <--- FIN SC
    return nbloque;
}

/**
 * liberar_bloque
 * ------
 * Libera un bloque determinado en el Mapa de Bits y actualiza el contador
 * en el Superbloque.
 * NOTA: Se podría optimizar (la función abre el SB 2 veces, una en escribir_bit y otra para actualizar el contador), 
 * pero se ha decidido mantener la simplicidad y claridad de la función como dicta el enunciado.
 * Parámetros:
 * - nbloque: número físico del bloque a liberar.
 * Retorno:
 * - Devuelve el número de bloque liberado, o FALLO (-1) en caso de error.
 * Llama a: escribir_bit(), bread(), bwrite()
 * Es llamada por: liberar_directos(), liberar_indirectos_recursivo(), liberar_bloques_inodo()
 */
int liberar_bloque(unsigned int nbloque)
{
    mi_waitSem(); // <--- INICIO SC

    if (escribir_bit(nbloque, 0) == FALLO) {
        mi_signalSem();
        return FALLO;  
    }

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    SB.cantBloquesLibres++;
    if (bwrite(posSB, &SB) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    mi_signalSem(); // <--- FIN SC
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

    // Vemos en qué bloque del AI (Array de Inodos) se encuentra el inodo que queremos escribir
    unsigned int nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;
    // Sumamos el bloque relativo al bloque donde empieza el AI para obtener la ruta física absoluta
    unsigned int nbloqueabs = SB.posPrimerBloqueAI + nbloqueAI;

    // Obtenemos el array de inodos
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
    mi_waitSem(); // <--- INICIO SC

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    if (SB.cantInodosLibres == 0) {
        fprintf(stderr, RED "Error: No hay inodos libres en el dispositivo.\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    // 1. Obtenemos el ID del primer inodo libre desde la cabecera del SB
    unsigned int posInodoReservado = SB.posPrimerInodoLibre;

    // 2. Leemos ese inodo para saber a cuál apunta (el siguiente libre)
    struct inodo inodoReservado;
    if (leer_inodo(posInodoReservado, &inodoReservado) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    // 3. Actualizamos el SB para que apunte al siguiente, que ahora será el primer inodo libre
    SB.posPrimerInodoLibre = inodoReservado.punterosDirectos[0];
    SB.cantInodosLibres--;
    if (bwrite(posSB, &SB) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

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
    if (escribir_inodo(posInodoReservado, &inodoReservado) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    mi_signalSem(); // <--- FIN SC
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
    // Los umbrales se calculan de forma acumulativa en base a los 256 punteros por bloque
    // DIRECTOS (12): Cubre del bloque lógico 0 al 11
    if (nblogico < DIRECTOS)
    {
        *ptr = inodo->punterosDirectos[nblogico];
        return 0; // Rango Directo
    }
    // INDIRECTOS0 (268): Suma los 12 directos + 256 punteros que caben en 1 bloque índice
    else if (nblogico < INDIRECTOS0)
    {
        *ptr = inodo->punterosIndirectos[0];
        return 1; // Indirecto Simple
    }
    // INDIRECTOS1 (65.804): Suma el umbral anterior (268) + (256 * 256) punteros del nivel doble
    else if (nblogico < INDIRECTOS1)
    {
        *ptr = inodo->punterosIndirectos[1];
        return 2; // Indirecto Doble
    }
    // INDIRECTOS2 (16.843.020): Suma el umbral anterior (65.804) + (256 * 256 * 256) del nivel triple
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
    // Nivel Directo. El índice es directamente el número de bloque (0-11)
    if (nblogico < DIRECTOS)
    {
        return nblogico; // Directo, no hay índice dentro de bloque
    }
    // Indirecto Simple. Restamos los 12 bloques directos para relativizar el índice al rango [0-255]
    else if (nblogico < INDIRECTOS0)
    {
        return nblogico - DIRECTOS; // Indirecto 0 (nivel 1)
    }
    // Indirecto Doble
    else if (nblogico < INDIRECTOS1)
    {
        if (nivel_punteros == 2)
        {
            // Dividimos entre 256 para saber en qué bloque intermedio caerá
            return (nblogico - INDIRECTOS0) / NPUNTEROS;
        }
        else if (nivel_punteros == 1)
        {
            // El módulo (%) con 256 nos da el resto, que es la posición exacta en el bloque final
            return (nblogico - INDIRECTOS0) % NPUNTEROS;
        }
    }
    // Indirecto Triple. Usamos la descomposición polinomial en base 256
    else if (nblogico < INDIRECTOS2)
    { // Indirecto 2
        if (nivel_punteros == 3)
        {
            // Índice superior: División entera entre 65536 (256^2)
            return (nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS);
        }
        else if (nivel_punteros == 2)
        {
            // Índice intermedio: Cogemos el resto de la división anterior y lo dividimos entre 256
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS;
        }
        else if (nivel_punteros == 1)
        {
            // Índice inferior (puntero a datos): Es el resto de dividir la parte intermedia entre 256
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
    // ptr será la "dirección actual" y ptr_ant la "dirección anterior" para ir navegando por los bloques de punteros
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
    // Esta variable nos servirá para saber a qué nivel de indirección estamos en cada momento del bucle
    nivel_punteros = nRangoBL; // Empezamos por el nivel más alto

    // Bucle de navegación de árbol.
    // Empezamos en el nivel más alto de indirección y vamos bajando (nivel_punteros--)
    // hasta llegar al nivel 0, que es donde se encuentra el bloque de datos real.
    while (nivel_punteros > 0)
    {
        if (ptr == 0) {
            // Hueco detectado (ptr == 0). Si 'reservar' es 0 (modo lectura), 
            // devolvemos fallo silencioso porque el bloque no existe
            if (reservar == 0)
                return FALLO; // Error si solo consultamos

            // Si 'reservar' es 1, creamos el bloque índice intermedio
            ptr = reservar_bloque();
            inodo.numBloquesOcupados++;
            inodo.ctime = time(NULL);
            salvar_inodo = 1; // Flag para no olvidar guardar el inodo al final

            // Si el bloque nuevo cuelga directamente del inodo
            if (nivel_punteros == nRangoBL)
            {
                // Si estamos en lo más alto del árbol, el puntero se guarda directamente en la estructura del inodo
                inodo.punterosIndirectos[nRangoBL - 1] = ptr;
                #if DEBUGN4 || DEBUGN5 || DEBUGN6
                fprintf(stderr, BLACK "[traducir_bloque_inodo() -> inodo.punterosIndirectos[%d] = %d (reservado BF %d para punteros_nivel%d)]\n" RESET,
                       nRangoBL - 1, ptr, ptr, nivel_punteros);
                #endif
            }
            else
            {
                // Si cuelga de otro bloque de punteros, actualizamos el buffer anterior y lo guardamos
                buffer[indice] = ptr;
                #if DEBUGN4 || DEBUGN5 || DEBUGN6
                fprintf(stderr, BLACK "[traducir_bloque_inodo() -> punteros_nivel%d [%d] = %d (reservado BF %d para punteros_nivel%d)]\n" RESET,
                       nivel_punteros + 1, indice, ptr, ptr, nivel_punteros);
                #endif
                // Este buffer no tiene por qué solo contener la dirección ptr_ant:
                // Si vamos un par de líneas más abajo, veremos que, si el bloque de punteros ya existía, 
                //lo leíamos en lugar de reservar uno nuevo. En ese caso, el buffer se habría llenado con las direcciones anteriores, 
                //y solo actualizaríamos la que nos interesa (buffer[indice] = ptr).
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

        // Avance iterativo. Calculamos el índice que nos toca explorar,
        // guardamos el bloque actual como 'padre' (ptr_ant), y avanzamos al hijo (ptr)
        indice = obtener_indice(nblogico, nivel_punteros);
        ptr_ant = ptr;        // Guardamos el bloque actual (padre)
        ptr = buffer[indice]; // Avanzamos al siguiente puntero (hijo)
        nivel_punteros--;
    }

    // Al salir del bucle while, ya estamos al nivel de los DATOS
    // Aplicamos exactamente la misma lógica: si es lectura fallamos, si es escritura reservamos
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
        { // Si era un puntero directo, lo guardamos en el array del inodo
            inodo.punterosDirectos[nblogico] = ptr;
            #if DEBUGN4 || DEBUGN5 || DEBUGN6
            fprintf(stderr, BLACK "[traducir_bloque_inodo() -> inodo.punterosDirectos[%d] = %d (reservado BF %d para BL %d)]\n" RESET,
                   nblogico, ptr, ptr, nblogico);
            #endif
        }
        else
        { // Si era indirecto, lo guardamos en el buffer del índice más bajo (nivel 1) y escribimos
            buffer[indice] = ptr;
            #if DEBUGN4 || DEBUGN5 || DEBUGN6
            fprintf(stderr, BLACK "[traducir_bloque_inodo() -> punteros_nivel1 [%d] = %d (reservado BF %d para BL %d)]\n" RESET,
                   indice, ptr, ptr, nblogico);
            #endif
            bwrite(ptr_ant, buffer); // Salvamos el bloque de punteros padre
        }
    }

    // Optimización: solo hacemos la pesada operación de escribir_inodo en disco si realmente hemos modificado algo.
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
 * Incrementa 'nBL' hasta 12, dejando el contador listo para que la zona de
 * indirectos sepa exactamente por dónde empezar.
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
            *eof = 1; // Si el archivo era pequeño, activamos EOF para que las funciones de indirectos no hagan nada
            break;
        }

        if (inodo->punterosDirectos[i] != 0)
        {
            liberar_bloque(inodo->punterosDirectos[i]);
            inodo->punterosDirectos[i] = 0;
            liberados++;
        }
        (*nBL)++; // nBL avanza de 1 en 1 porque son bloques de datos reales
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
 * - primerBL: bloque lógico inicial del rango a borrar.
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

    // Calculamos cuántos bloques lógicos abarca este nivel de punteros (1 bloque = NPUNTEROS hijos, 2 bloques = NPUNTEROS^2...)
    unsigned int saltos = 1;
    for (int k = 0; k < nivel_punteros; k++) {
        saltos *= NPUNTEROS;
    }

    // Si la rama está vacía o ya ha pasado el rango a borrar, saltamos el bloque
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

    // Tras limpiar a los hijos, comprobamos si el bloque de punteros ha quedado vacío (todo ceros)
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
        
            fprintf(stderr, ORANGE "[liberar_bloques_inodo() -> salvado BF %d de punteros_nivel%d correspondiente al BL %d]\n" RESET, *puntero_bloque, nivel_punteros, *nBL);
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
 * 
 * EXPLICACIÓN DEL ALGORITMO DE BORRADO
 * El borrado de bloques se hace "de abajo hacia arriba".
 * 1. Primero navegamos hasta el nivel más profundo (los bloques de datos).
 * 2. Liberamos el bloque de datos usando liberar_bloque().
 * 3. En la RAM, ponemos a 0 el puntero que apuntaba a ese dato dentro de su bloque padre.
 * 4. TRUCO DE OPTIMIZACIÓN (memcmp): Usamos memcmp para comparar el bloque padre entero
 * con un buffer auxiliar lleno de ceros. Si son idénticos, significa que hemos borrado 
 * TODOS los hijos de ese bloque índice. En ese caso, borramos también el bloque índice
 * y repetimos el proceso hacia arriba para no dejar "bloques huérfanos" ocupando espacio.
 * 
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
    mi_waitSem(); // <--- INICIO SC

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

    mi_signalSem(); // <--- FIN SC
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
    mi_waitSem(); // <--- INICIO SC
    struct inodo inodo;
    struct superbloque SB;

    // 1. Leer inodo
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    // 2. Liberar todos sus bloques de datos y punteros asociados
    // Pasamos primerBL = 0 porque queremos borrar todo el archivo
    int bloques_liberados = liberar_bloques_inodo(0, &inodo);
    inodo.numBloquesOcupados -= bloques_liberados;

    // 3. Reseteamos la estructura del inodo
    // Si ha ido bien, debería ser 0 (o quedarán los de metadatos si no los contamos igual)
    // Forzamos a 0 por seguridad y tipo libre
    inodo.tipo = 'l';
    inodo.tamEnBytesLog = 0;
    inodo.numBloquesOcupados = 0;

    // 4. Actualizar lista de inodos libres
    if (bread(posSB, &SB) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    // Lógica LIFO (Last In, First Out) (pila/lista enlazada)
    // Insertamos el inodo recién liberado en la CABEZA de la lista enlazada de inodos libres.
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
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }
    if (bwrite(posSB, &SB) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    #if DEBUGN6
    fprintf(stderr, "[liberar_inodo()→ Tras liberar inodo: primerInodoLibre=%d]\n", SB.posPrimerInodoLibre);
    #endif

    mi_signalSem(); // <--- FIN SC
    return ninodo;
}