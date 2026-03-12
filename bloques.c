// bloques.c
// Descripción: Implementación de las funciones de bajo nivel para gestión del disco virtual.

#include "bloques.h"

// Variable global estática para el descriptor del fichero.
// Al ser static, su visibilidad está restringida a este archivo.
static int descriptor = 0;

/**
 * bmount
 * ------
 * Función para montar el dispositivo virtual (fichero).
 * Parámetros:
 * - camino: cadena con la ruta/nombre del fichero a abrir/crear.
 * Retorno:
 * - Devuelve el descriptor del fichero o FALLO (-1) en caso de error.
 * Llama a: umask(), open(), perror()
 * Es llamada por: mi_mkfs(), leer_sf()
 */
int bmount(const char *camino) {
    // Configuración de permisos: rw-rw-rw- (0666)
    // Usamos umask(0000) para asegurar que el sistema no restrinja los permisos solicitados.
    umask(0000); 

    // Abrimos el fichero. Flags:
    // O_RDWR: Lectura y escritura.
    // O_CREAT: Si no existe, lo crea.
    // 0666: Permisos de lectura/escritura para usuario, grupo y otros.
    descriptor = open(camino, O_RDWR | O_CREAT, 0666);

    if (descriptor == -1) {
        perror(RED "Error en bmount al abrir el fichero" RESET);
        return FALLO;
    }

    return descriptor;
}

/**
 * bumount
 * -------
 * Desmonta el dispositivo virtual cerrando el fichero.
 * Parámetros:
 * - Ninguno.
 * Retorno:
 * - EXITO (0) si se cerró bien, FALLO (-1) si hubo error.
 * Llama a: close(), perror()
 * Es llamada por: mi_mkfs(), leer_sf
 */
int bumount() {
    // Intentamos cerrar el descriptor actual
    if (close(descriptor) == -1) {
        perror(RED "Error en bumount al cerrar el descriptor" RESET);
        return FALLO;
    }
    
    return EXITO;
}

/**
 * bwrite
 * ------
 * Escribe un bloque de datos en el dispositivo virtual.
 * Parámetros:
 * - nbloque: Número de bloque lógico donde escribir (índice empezando en 0).
 * - buf: Puntero al buffer de memoria con los datos (tamaño BLOCKSIZE).
 * Retorno:
 * - Número de bytes escritos (debería ser BLOCKSIZE) o FALLO (-1).
 * Llama a: lseek(), write(), perror()
 * Es llamada por: mi_mkfs(), leer_sf()
 */
int bwrite(unsigned int nbloque, const void *buf) {
    // 1. Calcular el desplazamiento (offset) en bytes
    off_t desplazamiento = (off_t)nbloque * BLOCKSIZE;

    // 2. Mover el puntero del fichero a la posición correcta
    if (lseek(descriptor, desplazamiento, SEEK_SET) == (off_t)-1) {
        perror(RED "Error en bwrite al mover el puntero (lseek)" RESET);
        return FALLO;
    }

    // 3. Escribir el contenido del buffer en el fichero
    size_t bytes_escritos = write(descriptor, buf, BLOCKSIZE);

    if (bytes_escritos == -1) {
        perror(RED "Error en bwrite al escribir (write)" RESET);
        return FALLO;
    }

    // Devolvemos la cantidad de bytes escritos (debe coincidir con BLOCKSIZE)
    return (int)bytes_escritos;
}

/**
 * bread
 * -----
 * Lee un bloque de datos del dispositivo virtual.
 * Parámetros:
 * - nbloque: Número de bloque lógico a leer.
 * - buf: Buffer donde se almacenarán los datos leídos.
 * Retorno:
 * - Número de bytes leídos o FALLO (-1).
 * Llama a: lseek(), read(), perror()
 * Es llamada por: leer_sf()
 */
int bread(unsigned int nbloque, void *buf) {
    // 1. Calcular el desplazamiento
    off_t desplazamiento = (off_t)nbloque * BLOCKSIZE;

    // 2. Posicionar el puntero
    if (lseek(descriptor, desplazamiento, SEEK_SET) == (off_t)-1) {
        perror(RED "Error en bread al mover el puntero (lseek)" RESET);
        return FALLO;
    }

    // 3. Leer del fichero y guardar en buf
    size_t bytes_leidos = read(descriptor, buf, BLOCKSIZE);

    if (bytes_leidos == -1) {
        perror(RED "Error en bread al leer (read)" RESET);
        return FALLO;
    }

    // Devolvemos la cantidad de bytes leídos
    return (int)bytes_leidos;
}