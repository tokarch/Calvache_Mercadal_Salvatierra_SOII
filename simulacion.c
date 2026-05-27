// simulacion.c
// Descripción: Programa que simula accesos concurrentes al SF (Nivel 12).

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
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

/**
 * reaper
 * ------
 * Función manejadora de señales (enterrador de procesos zombies).
 * Se ejecuta de forma asíncrona cuando un proceso hijo termina (recibe SIGCHLD).
 * Utiliza waitpid() con el flag WNOHANG para "recolectar" a los hijos terminados
 * sin bloquear la ejecución del proceso padre, incrementando el contador global.
 * * Parámetros:
 * - Ninguno.
 * * Retorno:
 * - Ninguno (void).
 * * Llama a: waitpid()
 * * Es llamada por: Sistema Operativo (al recibir SIGCHLD)
 */
void reaper()
{
    pid_t ended;
    // 1. waitpid() con WNOHANG recoge cualquier hijo terminado (-1) sin bloquear.
    // El bucle while permite procesar múltiples hijos si terminaron simultáneamente.
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0)
    {
        acabados++;
    }
}

/**
 * main (Comando simulacion)
 * ------
 * Programa encargado de simular accesos concurrentes al sistema de ficheros.
 * Crea 100 procesos hijos, cada uno de los cuales realiza 50 escrituras en
 * posiciones aleatorias de su propio fichero "prueba.dat", demostrando el
 * funcionamiento correcto de los semáforos y exclusión mutua.
 * Uso: ./simulacion <disco>
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: fprintf(), signal(), bmount(), time(), localtime(), sprintf(), mi_creat(), bumount(), printf(), fork(), getpid(), snprintf(), srand(), gettimeofday(), rand(), mi_write(), usleep(), exit(), pause()
 */
int main(int argc, char **argv)
{
    // 1. Verificación de argumentos
    if (argc != 2)
    {
        fprintf(stderr, "Sintaxis: %s <disco>\n", argv[0]);
        return FALLO;
    }

    // 2. Asociar la señal SIGCHLD al manejador 'reaper' (enterrador)
    signal(SIGCHLD, reaper);

    // 3. Montar el dispositivo virtual (Proceso Padre)
    if (bmount(argv[1]) == FALLO)
        return FALLO;

    // 4. Crear directorio de simulación raíz con el formato: /simul_aaaammddhhmmss/
    char camino_simul[TAMNOMBRE];
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    sprintf(camino_simul, "/simul_%04d%02d%02d%02d%02d%02d/",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec);

    if (mi_creat(camino_simul, 6) == FALLO)
    {
        fprintf(stderr, "Error creando directorio de simulacion\n");
        bumount();
        return FALLO;
    }
    printf("Directorio de simulacion: %s\n", camino_simul);

    printf("*** INICIANDO SIMULACION (%d procesos, %d escrituras cada uno) ***\n", NUMPROCESOS, NUMESCRITURAS);

    // 5. Bucle de creación de procesos hijos concurrentes
    for (int proceso = 1; proceso <= NUMPROCESOS; proceso++)
    {
        pid_t pid = fork();

        if (pid == 0)
        { // --- PROCESO HIJO ---
            // 6. Remontar dispositivo para evitar compartir el offset del descriptor de fichero con el padre/hermanos
            if (bmount(argv[1]) == FALLO)
                exit(1);

            // 7. Crear el directorio personal del proceso: /simul_.../proceso_PID/
            char camino_proceso[TAMNOMBRE * 2];
            sprintf(camino_proceso, "%sproceso_%d/", camino_simul, getpid());
            if (mi_creat(camino_proceso, 6) == FALLO)
            {
                fprintf(stderr, "Error creando directorio proceso %d\n", getpid());
                exit(1);
            }

            // 8. Crear fichero de registros del proceso: /simul_.../proceso_PID/prueba.dat
            char camino_fichero[TAMNOMBRE * 3];
            snprintf(camino_fichero, sizeof(camino_fichero), "%sprueba.dat", camino_proceso);
            if (mi_creat(camino_fichero, 6) == FALLO)
            {
                fprintf(stderr, "Error creando fichero proceso %d\n", getpid());
                exit(1);
            }

            // 9. Inicializar semilla aleatoria única basada en el tiempo y el PID del hijo
            srand(time(NULL) + getpid());

            // 10. Bucle de escrituras aleatorias para forzar concurrencia y probar semáforos
            for (int nescritura = 1; nescritura <= NUMESCRITURAS; nescritura++)
            {
                struct REGISTRO registro;
                gettimeofday(&registro.fecha, NULL);
                registro.pid = getpid();
                registro.nEscritura = nescritura;
                registro.nRegistro = rand() % REGMAX; // Posición aleatoria [0, REGMAX-1]

                // 11. Escribir registro en la posición calculada (offset = nRegistro * tamaño_registro)
                mi_write(camino_fichero, &registro,
                         registro.nRegistro * sizeof(struct REGISTRO),
                         sizeof(struct REGISTRO));

#if DEBUGN12
                fprintf(stderr, "[simulación.c → Escritura %d en %s]\n", nescritura, camino_fichero);
#endif

                // 12. Esperar 50ms para escalonar escrituras y simular carga real
                usleep(50000);
            }

            printf("[Proceso %d: Completadas %d escrituras en %s]\n", proceso, NUMESCRITURAS, camino_fichero);

            // 13. Desmontar dispositivo local y terminar proceso
            bumount();
            exit(0); // Lanza SIGCHLD al padre
        }

        // --- PROCESO PADRE ---
        // 14. Esperar 150ms antes de lanzar el siguiente hijo para no saturar al SO de golpe
        usleep(150000);
    }

    // 15. Bloquear al padre hasta que todos los hijos finalicen su ejecución
    while (acabados < NUMPROCESOS)
    {
        pause(); // Espera hasta recibir una señal (SIGCHLD)
    }

    printf("Todos los procesos han terminado.\n");

    // 16. Desmontar dispositivo y terminar simulación
    bumount();
    return EXITO;
}
