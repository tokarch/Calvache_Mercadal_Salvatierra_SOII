// semaforo_mutex_posix.c
// Descripción: Implementación de semáforos POSIX con nombre.

/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

VERIFICADO FINALMENTE Y COMENTADO POR
=====================================
Mercadal Forteza, Lluís
*/

#include "semaforo_mutex_posix.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

// Nombre único para el semáforo en el sistema
#define SEM_NAME "/sem_mi_sf" 

sem_t *initSem() {
    // Abrimos o creamos el semáforo. Valor inicial = 1 (Mutex)
    // O_CREAT: Crear si no existe.
    // 0644: Permisos.
    // 1: Valor inicial (un solo proceso entra a la vez).
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);
    
    if (sem == SEM_FAILED) {
        perror("Error en initSem");
        return SEM_FAILED;
    }
    return sem;
}

void deleteSem(sem_t *sem) {
    // Cerrar la referencia del proceso y desvincular el nombre
    if (sem_close(sem) == -1) {
        perror("Error en deleteSem (sem_close)");
    }
    // sem_unlink devuelve ENOENT si otro proceso ya lo desvinculó → esperado, no es error
    if (sem_unlink(SEM_NAME) == -1 && errno != ENOENT) {
        perror("Error en deleteSem (sem_unlink)");
    }
}

void signalSem(sem_t *sem) {
    if (sem_post(sem) == -1) {
        perror("Error en signalSem");
    }
}

void waitSem(sem_t *sem) {
    if (sem_wait(sem) == -1) {
        perror("Error en waitSem");
    }
}