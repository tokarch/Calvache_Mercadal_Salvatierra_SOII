// semaforo_mutex_posix.h
// Descripción: Cabecera para la gestión de semáforos POSIX.

/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE Y COMENTADO POR
=====================================
Mercadal Forteza, Lluís
*/

#ifndef SEMAFORO_MUTEX_POSIX_H
#define SEMAFORO_MUTEX_POSIX_H

#include <semaphore.h>

// Prototipos
sem_t *initSem();
void deleteSem(sem_t *sem);
void signalSem(sem_t *sem);
void waitSem(sem_t *sem);

#endif // SEMAFORO_MUTEX_POSIX_H