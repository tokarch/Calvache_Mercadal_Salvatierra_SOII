// ficheros_basico.h
// Descripción: Estructuras y prototipos para el manejo de metadatos (Nivel 2).

#ifndef FICHEROS_BASICO_H
#define FICHEROS_BASICO_H

#include "bloques.h"
#include <limits.h> // Para UINT_MAX
#include <time.h>   // Para time_t

// Constantes de diseño
#define posSB 0         // El superbloque está en el bloque 0
#define tamSB 1         // El superbloque ocupa 1 bloque
#define INODOSIZE 128   // Tamaño del inodo en bytes

// ----------------------------------------------------------------------
// SUPERBLOQUE
// ----------------------------------------------------------------------
struct superbloque {
    unsigned int posPrimerBloqueMB;    // Inicio Mapa de Bits
    unsigned int posUltimoBloqueMB;    // Fin Mapa de Bits
    unsigned int posPrimerBloqueAI;    // Inicio Array de Inodos
    unsigned int posUltimoBloqueAI;    // Fin Array de Inodos
    unsigned int posPrimerBloqueDatos; // Inicio Datos
    unsigned int posUltimoBloqueDatos; // Fin Datos
    unsigned int posInodoRaiz;         // Inodo del directorio raíz
    unsigned int posPrimerInodoLibre;  // Primer inodo de la lista de libres
    unsigned int cantBloquesLibres;    // Bloques disponibles
    unsigned int cantInodosLibres;     // Inodos disponibles
    unsigned int totBloques;           // Total bloques del disco
    unsigned int totInodos;            // Total inodos del disco
    char padding[BLOCKSIZE - 12 * sizeof(unsigned int)]; // Relleno
};

// ----------------------------------------------------------------------
// INODO
// ----------------------------------------------------------------------
struct inodo {
    unsigned char tipo;             // 'l': libre, 'd': directorio, 'f': fichero
    unsigned char permisos;         // rwx...
    unsigned char reservado_alineacion[6]; 

    time_t atime;                   // Último acceso
    time_t mtime;                   // Última modificación
    time_t ctime;                   // Cambio de estado (metadatos)
    time_t btime;                   // Creación (birth) (opcional según PDF, pero útil)

    unsigned int nlinks;            // Enlaces apuntando a este inodo
    unsigned int tamEnBytesLog;     // Tamaño lógico (EOF)
    unsigned int numBloquesOcupados;// Bloques físicos ocupados

    unsigned int punterosDirectos[12];
    unsigned int punterosIndirectos[3]; // Simple, Doble, Triple

    // Relleno para asegurar 128 bytes exactos.
    // Cálculo: 128 - (2 char + 6 res + 4 time_t + 3 int + 15 int) 
    // time_t suele ser 8 bytes en 64 bits.
    // 128 - (8 + 32 + 12 + 60) = 128 - 112 = 16 bytes aprox. 
    // Ajustaremos esto dinámicamente o confiamos en el cálculo manual:
    // sizeof(struct inodo) debe ser 128.
    char padding[INODOSIZE - 2 * sizeof(unsigned char) - 6 * sizeof(unsigned char) - 
                 4 * sizeof(time_t) - 18 * sizeof(unsigned int)];
};

// Prototipos de funciones
int tamMB(unsigned int nbloques);
int tamAI(unsigned int ninodos);
int initSB(unsigned int nbloques, unsigned int ninodos);
int initMB();
int initAI();

int escribir_bit(unsigned int nbloque, unsigned int bit);
char leer_bit(unsigned int nbloque);
int reservar_bloque();
int liberar_bloque(unsigned int nbloque);
int escribir_inodo(unsigned int ninodo, struct inodo *inodo);
int leer_inodo(unsigned int ninodo, struct inodo *inodo);
int reservar_inodo(unsigned char tipo, unsigned char permisos);

#endif // FICHEROS_BASICO_H