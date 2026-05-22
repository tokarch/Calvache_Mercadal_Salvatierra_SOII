// simulacion.c
// Descripción: Programa que simula accesos concurrentes al SF (Nivel 12).

/*
PROGRAMADO, REVISADO Y VERIFICADO POR
=====================================
Calvache Vidal, Antonio

REVISADO (SEGUNDA PASADA) POR
=============================
Salvatierra Romero, Diego Jeroni

VERIFICADO FINALMENTE POR
=====================================
Mercadal Forteza, Lluís
*/

#include "directorios.h"
#include "simulacion.h"
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

int acabados = 0; // Contador de procesos finalizados

// Enterrador de procesos zombies
void reaper() {
    pid_t ended;
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0) {
        acabados++;
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Sintaxis: %s <disco>\n", argv[0]);
        return FALLO;
    }

    // Asociar la señal SIGCHLD al enterrador
    signal(SIGCHLD, reaper);

    // 1. Montar dispositivo (Padre)
    if (bmount(argv[1]) == FALLO) return FALLO;

    // 2. Crear directorio de simulación: /simul_aaaammddhhmmss/
    char camino_simul[TAMNOMBRE];
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    sprintf(camino_simul, "/simul_%04d%02d%02d%02d%02d%02d/", 
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, 
            tm->tm_hour, tm->tm_min, tm->tm_sec);
    
    if (mi_creat(camino_simul, 6) == FALLO) {
        fprintf(stderr, "Error creando directorio de simulacion\n");
        bumount();
        return FALLO;
    }
    printf("Directorio de simulacion: %s\n", camino_simul);

    printf("*** INICIANDO SIMULACION (%d procesos, %d escrituras cada uno) ***\n", NUMPROCESOS, NUMESCRITURAS);

    // 3. Bucle de creación de procesos
    for (int proceso = 1; proceso <= NUMPROCESOS; proceso++) {
        pid_t pid = fork();

        if (pid == 0) { // --- PROCESO HIJO ---
            // Remontar dispositivo para tener descriptor propio
            if (bmount(argv[1]) == FALLO) exit(1);

            // Crear directorio del proceso: /simul_.../proceso_PID/
            char camino_proceso[TAMNOMBRE * 2];
            sprintf(camino_proceso, "%sproceso_%d/", camino_simul, getpid());
            if (mi_creat(camino_proceso, 6) == FALLO) {
                fprintf(stderr, "Error creando directorio proceso %d\n", getpid());
                exit(1);
            }

            // Crear fichero prueba.dat: /simul_.../proceso_PID/prueba.dat
            char camino_fichero[TAMNOMBRE * 3];
            snprintf(camino_fichero, sizeof(camino_fichero), "%sprueba.dat", camino_proceso);
            if (mi_creat(camino_fichero, 6) == FALLO) {
                fprintf(stderr, "Error creando fichero proceso %d\n", getpid());
                exit(1);
            }

            // Inicializar semilla aleatoria única
            srand(time(NULL) + getpid());

            // Bucle de escrituras
            for (int nescritura = 1; nescritura <= NUMESCRITURAS; nescritura++) {
                struct REGISTRO registro;
                gettimeofday(&registro.fecha, NULL);
                registro.pid = getpid();
                registro.nEscritura = nescritura;
                registro.nRegistro = rand() % REGMAX; // Posición aleatoria [0, REGMAX-1]

                // Escribir registro en la posición correspondiente
                // offset = nRegistro * sizeof(REGISTRO)
                mi_write(camino_fichero, &registro, 
                         registro.nRegistro * sizeof(struct REGISTRO), 
                         sizeof(struct REGISTRO));

                // Esperar 0.05 segundos
                usleep(50000); 
            }

            printf("[Proceso %d: Completadas %d escrituras en %s]\n", proceso, NUMESCRITURAS, camino_fichero);

            // Desmontar y salir
            bumount();
            exit(0); // Lanza SIGCHLD al padre
        }

        // --- PROCESO PADRE ---
        // Esperar 0.15 segundos antes de lanzar el siguiente
        usleep(150000);
    }

    // 4. Esperar a que terminen todos los hijos
    while (acabados < NUMPROCESOS) {
        pause(); // Espera hasta recibir una señal (SIGCHLD)
    }

    printf("Todos los procesos han terminado.\n");

    // 5. Desmontar (Padre)
    bumount();
    return EXITO;
}
