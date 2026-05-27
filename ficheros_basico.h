// ficheros_basico.h
// Descripción: Cabecera actualizada con constantes de punteros (Nivel 4).

#ifndef FICHEROS_BASICO_H
#define FICHEROS_BASICO_H

#include "bloques.h"
#include <limits.h> // Para UINT_MAX
#include <time.h>   // Para time_t

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

// Constantes de diseño
#define posSB 0       // El superbloque está en el bloque 0
#define tamSB 1       // El superbloque ocupa 1 bloque
#define INODOSIZE 128 // Tamaño del inodo en bytes

// CONSTANTES PARA EL DIRECCIONAMIENTO DE INODOS (NIVEL 4)
#define NPUNTEROS (BLOCKSIZE / sizeof(unsigned int)) // 256 punteros por bloque
#define DIRECTOS 12
#define INDIRECTOS0 (NPUNTEROS + DIRECTOS)                            // 268
#define INDIRECTOS1 (NPUNTEROS * NPUNTEROS + INDIRECTOS0)             // 65.804
#define INDIRECTOS2 (NPUNTEROS * NPUNTEROS * NPUNTEROS + INDIRECTOS1) // 16.843.020

// ----------------------------------------------------------------------
// SUPERBLOQUE
// ----------------------------------------------------------------------
struct superbloque
{
    unsigned int posPrimerBloqueMB;                      // Posición absoluta del primer bloque del mapa de bits
    unsigned int posUltimoBloqueMB;                      // Posición absoluta del último bloque del mapa de bits
    unsigned int posPrimerBloqueAI;                      // Posición absoluta del primer bloque del array de inodos
    unsigned int posUltimoBloqueAI;                      // Posición absoluta del último bloque del array de inodos
    unsigned int posPrimerBloqueDatos;                   // Posición absoluta del primer bloque de datos
    unsigned int posUltimoBloqueDatos;                   // Posición absoluta del último bloque de datos
    unsigned int posInodoRaiz;                           // Posición del inodo del directorio raíz (relativa al AI)
    unsigned int posPrimerInodoLibre;                    // Posición del primer inodo libre (relativa al AI)
    unsigned int cantBloquesLibres;                      // Cantidad de bloques libres (en todo el disco)
    unsigned int cantInodosLibres;                       // Cantidad de inodos libres (en el AI)
    unsigned int totBloques;                             // Cantidad total de bloques del disco
    unsigned int totInodos;                              // Cantidad total de inodos (heurística)
    char padding[BLOCKSIZE - 12 * sizeof(unsigned int)]; // Relleno para ocupar el bloque completo
};

// ----------------------------------------------------------------------
// INODO
// ----------------------------------------------------------------------
struct inodo
{
    unsigned char tipo;                    // 'l': libre, 'd': directorio, 'f': fichero
    unsigned char permisos;                // rwx (lectura y/o escritura y/o ejecución)...
    unsigned char reservado_alineacion[6]; // Microprocesador de 64 bits

    time_t atime; // Último acceso a datos (contenido)
    time_t mtime; // Última modificación de datos (contenido)
    time_t ctime; // Última modificación del inodo
    time_t btime; // Creación del inodo (birth) (opcional según PDF, pero útil)

    unsigned int nlinks;             // Enlaces de entradas en directorio apuntando a este inodo
    unsigned int tamEnBytesLog;      // Tamaño en bytes lógicos (EOF)
    unsigned int numBloquesOcupados; // Cantidad de bloques ocupados en zona de datos

    unsigned int punterosDirectos[12];  // 12 punteros a bloques directos
    unsigned int punterosIndirectos[3]; // Indirecto simple, doble, triple

    // Relleno para asegurar 128 bytes exactos.
    // Cálculo: 128 - (2 char + 6 res + 4 time_t + 3 int + 15 int)
    // time_t suele ser 8 bytes en 64 bits.
    // 128 - (8 + 32 + 12 + 60) = 128 - 112 = 16 bytes aprox.
    // Ajustaremos esto dinámicamente o confiamos en el cálculo manual:
    // sizeof(struct inodo) debe ser 128.
    char padding[INODOSIZE - 2 * sizeof(unsigned char) - 6 * sizeof(unsigned char) -
                 4 * sizeof(time_t) - 18 * sizeof(unsigned int)];
};

// Funciones Nivel 2
int tamMB(unsigned int nbloques);
int tamAI(unsigned int ninodos);
int initSB(unsigned int nbloques, unsigned int ninodos);
int initMB();
int initAI();

// Funciones Nivel 3
int escribir_bit(unsigned int nbloque, unsigned int bit);
char leer_bit(unsigned int nbloque);
int reservar_bloque();
int liberar_bloque(unsigned int nbloque);
int escribir_inodo(unsigned int ninodo, struct inodo *inodo);
int leer_inodo(unsigned int ninodo, struct inodo *inodo);
int reservar_inodo(unsigned char tipo, unsigned char permisos);

// Funciones Nivel 4
int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, unsigned char reservar);

// Funciones Nivel 6 (NUEVAS)
int liberar_inodo(unsigned int ninodo);
int liberar_bloques_inodo(unsigned int primerBL, struct inodo *inodo);

#endif // FICHEROS_BASICO_H