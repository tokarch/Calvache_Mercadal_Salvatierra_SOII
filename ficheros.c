// ficheros.c
// Descripción: Implementación de operaciones de lectura/escritura a nivel de fichero.

#include "ficheros.h"

/**
 * mi_write_f
 * Escribe contenido de un buffer en un fichero (inodo) a partir de un offset.
 * Gestiona la fragmentación en bloques lógicos y físicos.
 */
int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    struct inodo inodo;
    unsigned int primerBL, ultimoBL;
    unsigned int desp1, desp2;
    unsigned int nbfisico;
    unsigned char buf_bloque[BLOCKSIZE];
    int bytes_escritos = 0;

    // 1. Leemos el inodo para verificar permisos y actualizar metadatos
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;

    // Verificamos permisos de escritura (bit 'w' -> 2)
    if ((inodo.permisos & 2) != 2) {
        fprintf(stderr, "Error: No hay permisos de escritura en inodo %d\n", ninodo);
        return FALLO;
    }

    // 2. Calculamos bloques afectados
    primerBL = offset / BLOCKSIZE;
    ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;

    // Desplazamientos dentro de los bloques
    desp1 = offset % BLOCKSIZE;
    desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    // 3. Caso 1: La escritura cabe en un solo bloque lógico
    if (primerBL == ultimoBL) {
        // Obtenemos el bloque físico (reservando si no existe)
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
        if (nbfisico == -1) return FALLO;

        // Leemos el bloque físico (read-modify-write) para preservar datos no sobreescritos
        if (bread(nbfisico, buf_bloque) == FALLO) return FALLO;

        // Copiamos los datos del buffer usuario al buffer de bloque en la posición correcta
        memcpy(buf_bloque + desp1, buf_original, nbytes);

        // Escribimos el bloque modificado en disco
        if (bwrite(nbfisico, buf_bloque) == FALLO) return FALLO;
        
        bytes_escritos = nbytes;
    } 
    // 4. Caso 2: La escritura ocupa más de un bloque
    else {
        // --- FASE 1: Primer bloque (parcial) ---
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
        if (nbfisico == -1) return FALLO;

        if (bread(nbfisico, buf_bloque) == FALLO) return FALLO;
        
        // Escribimos desde desp1 hasta el final del bloque
        memcpy(buf_bloque + desp1, buf_original, BLOCKSIZE - desp1);
        
        if (bwrite(nbfisico, buf_bloque) == FALLO) return FALLO;
        
        bytes_escritos += BLOCKSIZE - desp1;

        // --- FASE 2: Bloques intermedios (completos) ---
        for (unsigned int bl = primerBL + 1; bl < ultimoBL; bl++) {
            nbfisico = traducir_bloque_inodo(ninodo, bl, 1);
            if (nbfisico == -1) return FALLO;

            // Escribimos directamente del buffer original (cálculo de punteros)
            if (bwrite(nbfisico, buf_original + (BLOCKSIZE - desp1) + (bl - primerBL - 1) * BLOCKSIZE) == FALLO) return FALLO;
            
            bytes_escritos += BLOCKSIZE;
        }

        // --- FASE 3: Último bloque (parcial) ---
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 1);
        if (nbfisico == -1) return FALLO;

        if (bread(nbfisico, buf_bloque) == FALLO) return FALLO;
        
        // Escribimos desde el principio del bloque hasta desp2
        memcpy(buf_bloque, buf_original + (nbytes - (desp2 + 1)), desp2 + 1);
        
        if (bwrite(nbfisico, buf_bloque) == FALLO) return FALLO;
        
        bytes_escritos += desp2 + 1;
    }

    // 5. Actualizar metadatos del inodo
    // Si hemos escrito más allá del EOF actual, actualizamos el tamaño
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO; // Releer inodo actualizado por traducir_bloque
    
    if (offset + nbytes > inodo.tamEnBytesLog) {
        inodo.tamEnBytesLog = offset + nbytes;
        inodo.ctime = time(NULL);
    }
    inodo.mtime = time(NULL);
    
    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO;

    return bytes_escritos;
}

/**
 * mi_read_f
 * Lee contenido de un fichero (inodo) y lo vuelca en un buffer.
 */
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    struct inodo inodo;
    unsigned int primerBL, ultimoBL;
    unsigned int desp1, desp2;
    unsigned int nbfisico;
    unsigned char buf_bloque[BLOCKSIZE];
    int bytes_leidos = 0;

    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;

    // Verificar permisos de lectura (bit 'r' -> 4)
    if ((inodo.permisos & 4) != 4) {
        fprintf(stderr, "Error: No hay permisos de lectura en inodo %d\n", ninodo);
        return FALLO;
    }

    // Comprobar que no intentamos leer más allá del EOF
    if (offset >= inodo.tamEnBytesLog) {
        return 0; // Nada que leer
    }
    if (offset + nbytes >= inodo.tamEnBytesLog) {
        nbytes = inodo.tamEnBytesLog - offset; // Ajustamos nbytes al final del fichero
    }

    primerBL = offset / BLOCKSIZE;
    ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
    desp1 = offset % BLOCKSIZE;
    desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    // Caso 1: Todo en un bloque
    if (primerBL == ultimoBL) {
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0); // Solo consultar
        
        if (nbfisico != -1) {
            if (bread(nbfisico, buf_bloque) == FALLO) return FALLO;
            memcpy(buf_original, buf_bloque + desp1, nbytes);
        } else {
            // Hueco en fichero disperso: rellenar con ceros
            memset(buf_original, 0, nbytes);
        }
        bytes_leidos = nbytes;
    } 
    // Caso 2: Varios bloques
    else {
        // --- FASE 1: Primer bloque ---
        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);
        if (nbfisico != -1) {
            if (bread(nbfisico, buf_bloque) == FALLO) return FALLO;
            memcpy(buf_original, buf_bloque + desp1, BLOCKSIZE - desp1);
        } else {
            memset(buf_original, 0, BLOCKSIZE - desp1);
        }
        bytes_leidos += BLOCKSIZE - desp1;

        // --- FASE 2: Bloques intermedios ---
        for (unsigned int bl = primerBL + 1; bl < ultimoBL; bl++) {
            nbfisico = traducir_bloque_inodo(ninodo, bl, 0);
            if (nbfisico != -1) {
                if (bread(nbfisico, buf_bloque) == FALLO) return FALLO;
                memcpy(buf_original + (BLOCKSIZE - desp1) + (bl - primerBL - 1) * BLOCKSIZE, buf_bloque, BLOCKSIZE);
            } else {
                memset(buf_original + (BLOCKSIZE - desp1) + (bl - primerBL - 1) * BLOCKSIZE, 0, BLOCKSIZE);
            }
            bytes_leidos += BLOCKSIZE;
        }

        // --- FASE 3: Último bloque ---
        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 0);
        if (nbfisico != -1) {
            if (bread(nbfisico, buf_bloque) == FALLO) return FALLO;
            memcpy(buf_original + (nbytes - (desp2 + 1)), buf_bloque, desp2 + 1);
        } else {
            memset(buf_original + (nbytes - (desp2 + 1)), 0, desp2 + 1);
        }
        bytes_leidos += desp2 + 1;
    }

    // Actualizar atime
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;
    inodo.atime = time(NULL);
    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO;

    return bytes_leidos;
}

/**
 * mi_stat_f
 * Obtiene la metainformación de un fichero.
 */
int mi_stat_f(unsigned int ninodo, struct STAT *p_stat) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;

    p_stat->tipo = inodo.tipo;
    p_stat->permisos = inodo.permisos;
    p_stat->atime = inodo.atime;
    p_stat->mtime = inodo.mtime;
    p_stat->ctime = inodo.ctime;
    p_stat->nlinks = inodo.nlinks;
    p_stat->tamEnBytesLog = inodo.tamEnBytesLog;
    p_stat->numBloquesOcupados = inodo.numBloquesOcupados;

    return EXITO;
}

/**
 * mi_chmod_f
 * Cambia los permisos de un inodo.
 */
int mi_chmod_f(unsigned int ninodo, unsigned char permisos) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;

    inodo.permisos = permisos;
    inodo.ctime = time(NULL);

    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO;

    return EXITO;
}

// ==============================================================================
// FUNCIONES NIVEL 6
// ==============================================================================
/**
 * mi_truncar_f
 * Trunca un fichero a nbytes, liberando los bloques excedentes.
 */
int mi_truncar_f(unsigned int ninodo, unsigned int nbytes) {
    struct inodo inodo;
    int primerBL;
    int liberados;

    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;

    // Verificar permisos escritura
    if ((inodo.permisos & 2) != 2) {
        fprintf(stderr, "Error: Sin permisos de escritura en inodo %d\n", ninodo);
        return FALLO;
    }

    // No se puede truncar más allá del tamaño actual
    if (nbytes > inodo.tamEnBytesLog) {
        fprintf(stderr, "Error: No se puede truncar más allá del EOF\n");
        return FALLO;
    }

    // Calcular el primer bloque lógico a liberar
    if (nbytes % BLOCKSIZE == 0) {
        primerBL = nbytes / BLOCKSIZE;
    } else {
        primerBL = nbytes / BLOCKSIZE + 1;
    }

    // Llamar a liberar bloques
    liberados = liberar_bloques_inodo(primerBL, &inodo);

    // Actualizar inodo
    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);
    inodo.tamEnBytesLog = nbytes;
    inodo.numBloquesOcupados -= liberados;

    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO;

    return liberados;
}