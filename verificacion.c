// verificacion.c
// Descripción: Programa que verifica la coherencia de la simulación y genera un informe.

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

/*
 * Propósito: Verificar la coherencia de una simulación de concurrencia previa.
 * Recorre los directorios de cada proceso, analiza prueba.dat y genera informe.txt.
 * Uso: ./verificacion <disco> <directorio_simulacion>
 */
#include "directorios.h"
#include "verificacion.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

// Tamaño del buffer de lectura (debe ser múltiplo de sizeof(struct REGISTRO))
// para garantizar que cada bloque leído contiene registros completos,
// evitando cortes a mitad de estructura que complicarían el parseo.
// 256 registros * 24 bytes = 6144 bytes por lectura (eficiente)
#define CANT_REGISTROS_BUFFER 256
#define TAM_BUFFER (CANT_REGISTROS_BUFFER * sizeof(struct REGISTRO))

/**
 * main (Comando verificacion)
 * ------
 * Programa encargado de verificar la coherencia de una simulación de concurrencia previa.
 * Recorre los directorios de cada proceso, analiza el contenido de sus ficheros 'prueba.dat',
 * y genera un informe detallado ('informe.txt') con las métricas de escritura.
 * Uso: ./verificacion <disco> <directorio_simulacion>
 * * Parámetros (Línea de comandos):
 * - argv[1]: Nombre del dispositivo virtual (ej. "disco").
 * - argv[2]: Ruta absoluta del directorio de simulación a verificar (ej. "/simul_20230528123000/").
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o FALLO (-1) en caso de error.
 * * Llama a: fprintf(), bmount(), mi_stat(), mi_creat(), bumount(), malloc(), mi_read(), free(), strchr(), atoi(), sprintf(), localtime(), strftime(), strcat(), printf(), mi_write()
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: %s <disco> <directorio_simulacion>\n", argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *dir_simulacion = argv[2];

    // 1. Montar dispositivo
    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    // 2. Comprobar que existe el directorio de simulación
    struct STAT stat_dir;
    if (mi_stat(dir_simulacion, &stat_dir) == FALLO) {
        fprintf(stderr, "Error: No existe el directorio %s\n", dir_simulacion);
        bumount();
        return FALLO;
    }

    printf("dir_sim: %s\n", dir_simulacion);

    // 3. Crear el fichero informe.txt DENTRO del sistema de ficheros
    char camino_informe[TAMNOMBRE * 2];
    sprintf(camino_informe, "%sinforme.txt", dir_simulacion);
    if (mi_creat(camino_informe, 6) == FALLO) {
        fprintf(stderr, "Error creando informe.txt\n");
        bumount();
        return FALLO;
    }

    // 4. Leer las entradas del directorio de simulación.
    // Cada entrada representa un proceso (directorio "proceso_PID"), por lo que
    // dividimos el tamaño lógico del directorio entre el tamaño de entrada
    // para saber cuántos procesos debemos verificar.
    int num_entradas = stat_dir.tamEnBytesLog / sizeof(struct entrada);
    if (num_entradas == 0) {
        fprintf(stderr, "Directorio vacio.\n");
        bumount();
        return FALLO;
    }

    struct entrada *entradas = malloc(stat_dir.tamEnBytesLog);
    if (mi_read(dir_simulacion, entradas, 0, stat_dir.tamEnBytesLog) < 0) {
        fprintf(stderr, "Error leyendo directorio simulacion\n");
        free(entradas);
        bumount();
        return FALLO;
    }

    // Buffer para leer registros de prueba.dat
    struct REGISTRO buffer_registros[CANT_REGISTROS_BUFFER];

    printf("Iniciando verificacion de %d procesos...\n", num_entradas);

    // 5. Recorrer cada entrada (cada proceso)
    for (int i = 0; i < num_entradas; i++) {

        // Saltamos entradas inválidas o el propio informe.txt
        if (strchr(entradas[i].nombre, '_') == NULL) continue;

        // Extraer PID del nombre "proceso_1234"
        char *pid_str = strchr(entradas[i].nombre, '_') + 1;
        int pid = atoi(pid_str);

        // Estructura que acumula las métricas clave de un proceso para el informe:
        // PrimeraEscritura/UltimaEscritura: extremos temporales (desempate por timestamp)
        // MenorPosicion/MayorPosicion: extremos del rango de registros tocados
        // nEscrituras: conteo total de registros válidos encontrados
        struct INFORMACION info;
        info.pid = pid;
        info.nEscrituras = 0;

        // Indica si ya se ha encontrado el primer registro válido de este proceso
        // para inicializar los cuatro extremos (primera, última, menor, mayor).
        int primer_registro_valido = 0;

        // Construir ruta fichero prueba.dat
        char camino_fichero[TAMNOMBRE * 3];
        sprintf(camino_fichero, "%s%s/prueba.dat", dir_simulacion, entradas[i].nombre);

        // Obtener tamaño para el bucle de lectura
        struct STAT stat_fichero;
        if (mi_stat(camino_fichero, &stat_fichero) == FALLO) {
            continue;
        }

        int offset = 0;
        int leidos;

        // Bucle de lectura secuencial del fichero prueba.dat.
        // Se procesa por bloques de tamaño múltiplo de registro para eficiencia de E/S.
        while (offset < stat_fichero.tamEnBytesLog) {
            leidos = mi_read(camino_fichero, buffer_registros, offset, TAM_BUFFER);

            int num_regs_leidos = leidos / sizeof(struct REGISTRO);

            for (int j = 0; j < num_regs_leidos; j++) {
                struct REGISTRO reg = buffer_registros[j];

                // Verificación de coherencia: se comprueba que el PID coincide con el
                // del directorio (evita registros corruptos o escrituras cruzadas) y se
                // rastrean la primera y última escritura (por nEscritura, desempate por
                // fecha), junto con los extremos del rango de posiciones (menor/mayor nRegistro).
                // VALIDACIÓN: El PID debe coincidir
                if (reg.pid == pid) {

                    // Inicializa los cuatro extremos con el primer registro válido
                    if (!primer_registro_valido) {
                        info.PrimeraEscritura = reg;
                        info.UltimaEscritura = reg;
                        info.MenorPosicion = reg;
                        info.MayorPosicion = reg;
                        primer_registro_valido = 1;
                    } else {
                        // Primera Escritura (menor nEscritura, desempate por fecha)
                        if (reg.nEscritura < info.PrimeraEscritura.nEscritura) {
                            info.PrimeraEscritura = reg;
                        } else if (reg.nEscritura == info.PrimeraEscritura.nEscritura) {
                            if (reg.fecha.tv_sec < info.PrimeraEscritura.fecha.tv_sec ||
                               (reg.fecha.tv_sec == info.PrimeraEscritura.fecha.tv_sec &&
                                reg.fecha.tv_usec < info.PrimeraEscritura.fecha.tv_usec))
                                info.PrimeraEscritura = reg;
                        }

                        // Última Escritura (mayor nEscritura, desempate por fecha)
                        if (reg.nEscritura > info.UltimaEscritura.nEscritura) {
                            info.UltimaEscritura = reg;
                        } else if (reg.nEscritura == info.UltimaEscritura.nEscritura) {
                            if (reg.fecha.tv_sec > info.UltimaEscritura.fecha.tv_sec ||
                               (reg.fecha.tv_sec == info.UltimaEscritura.fecha.tv_sec &&
                                reg.fecha.tv_usec > info.UltimaEscritura.fecha.tv_usec))
                                info.UltimaEscritura = reg;
                        }

                        // Menor Posición (menor nRegistro)
                        if (reg.nRegistro < info.MenorPosicion.nRegistro) {
                            info.MenorPosicion = reg;
                        }

                        // Mayor Posición (mayor nRegistro)
                        if (reg.nRegistro > info.MayorPosicion.nRegistro) {
                            info.MayorPosicion = reg;
                        }
                    }
                    // Contador total de registros válidos verificados para este proceso
                    info.nEscrituras++;
                }
            }
            offset += leidos;
            if (leidos < TAM_BUFFER) break;
        }

        // --- Generar texto para el informe ---
        // Formato por proceso: PID, número de escrituras, primera, última,
        // menor posición y mayor posición (con fecha completa hasta microsegundos).
        // Se acumula en buffer para minimizar las llamadas de escritura a informe.txt.
        char datos_proceso[1024];
        sprintf(datos_proceso, "PID: %d\n", info.pid);
        sprintf(datos_proceso + strlen(datos_proceso), "Numero de escrituras: %d\n", info.nEscrituras);

        if (info.nEscrituras > 0) {
            char fecha_str[100];
            struct tm *tm;

            // Primera
            // Primera
            tm = localtime(&info.PrimeraEscritura.fecha.tv_sec);
            strftime(fecha_str, sizeof(fecha_str), "%a %Y-%m-%d %H:%M:%S", tm);
            sprintf(fecha_str + strlen(fecha_str), ".%06ld", info.PrimeraEscritura.fecha.tv_usec);
            sprintf(datos_proceso + strlen(datos_proceso),
                "Primera escritura: \t%d\t%d\t%s\n",
                info.PrimeraEscritura.nEscritura, info.PrimeraEscritura.nRegistro, fecha_str);

            // Última
            tm = localtime(&info.UltimaEscritura.fecha.tv_sec);
            strftime(fecha_str, sizeof(fecha_str), "%a %Y-%m-%d %H:%M:%S", tm);
            sprintf(fecha_str + strlen(fecha_str), ".%06ld", info.UltimaEscritura.fecha.tv_usec);
            sprintf(datos_proceso + strlen(datos_proceso),
                "Ultima escritura: \t%d\t%d\t%s\n",
                info.UltimaEscritura.nEscritura, info.UltimaEscritura.nRegistro, fecha_str);

            // Menor Posición
            tm = localtime(&info.MenorPosicion.fecha.tv_sec);
            strftime(fecha_str, sizeof(fecha_str), "%a %Y-%m-%d %H:%M:%S", tm);
            sprintf(fecha_str + strlen(fecha_str), ".%06ld", info.MenorPosicion.fecha.tv_usec);
            sprintf(datos_proceso + strlen(datos_proceso),
                "Menor posicion: \t%d\t%d\t%s\n",
                info.MenorPosicion.nEscritura, info.MenorPosicion.nRegistro, fecha_str);

            // Mayor Posición
            tm = localtime(&info.MayorPosicion.fecha.tv_sec);
            strftime(fecha_str, sizeof(fecha_str), "%a %Y-%m-%d %H:%M:%S", tm);
            sprintf(fecha_str + strlen(fecha_str), ".%06ld", info.MayorPosicion.fecha.tv_usec);
            sprintf(datos_proceso + strlen(datos_proceso),
                "Mayor posicion: \t%d\t%d\t%s\n",
                info.MayorPosicion.nEscritura, info.MayorPosicion.nRegistro, fecha_str);
        }
        strcat(datos_proceso, "\n");

        // Mostrar por pantalla
        printf("%s", datos_proceso);

        // Usamos i+1 porque 'i' es el contador del bucle for que recorre las entradas
        printf("[%d) %d escrituras validadas en %s]\n", i + 1, info.nEscrituras, camino_fichero);

        // Añadir al informe.txt (append).
        // Se obtiene el tamaño actual para escribir al final y concatenar cada proceso.
        struct STAT stat_informe;
        mi_stat(camino_informe, &stat_informe);
        mi_write(camino_informe, datos_proceso, stat_informe.tamEnBytesLog, strlen(datos_proceso));
    }

    free(entradas);
    bumount();
    printf("Verificacion finalizada. Informe guardado en %s\n", camino_informe);
    return EXITO;
}
