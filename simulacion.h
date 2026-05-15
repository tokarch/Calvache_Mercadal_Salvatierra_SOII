// simulacion.h
// Descripción: Definiciones para la simulación de concurrencia (Nivel 12).

/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE Y COMENTADO POR
=====================================
Mercadal Forteza, Lluís
*/

#ifndef SIMULACION_H
#define SIMULACION_H

#include <sys/types.h> // Para pid_t
#include <sys/time.h>  // Para struct timeval, gettimeofday()
#include <time.h>

// Constantes del enunciado
#define NUMPROCESOS 100
#define NUMESCRITURAS 50
#define REGMAX 500000 // Rango máximo de registros

struct REGISTRO { // sizeof(struct REGISTRO): 32 bytes
    struct timeval fecha; // Fecha de la escritura con microsegundos
    pid_t pid;            // PID del proceso
    int nEscritura;       // Número de escritura (1 a 50)
    int nRegistro;        // Posición del registro (0 a REGMAX-1)
};

#endif // SIMULACION_H
