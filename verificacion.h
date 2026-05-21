// verificacion.h
// Descripción: Estructuras para el informe de verificación (Nivel 13).

/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE POR
=====================================
Mercadal Forteza, Lluís
*/

#ifndef VERIFICACION_H
#define VERIFICACION_H

#include "simulacion.h"

struct INFORMACION {
    int pid;
    unsigned int nEscrituras;
    struct REGISTRO PrimeraEscritura;
    struct REGISTRO UltimaEscritura;
    struct REGISTRO MenorPosicion;
    struct REGISTRO MayorPosicion;
};

#endif // VERIFICACION_H
