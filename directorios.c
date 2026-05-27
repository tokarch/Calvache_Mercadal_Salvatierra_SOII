// directorios.c
// Descripción: Implementación de la capa de directorios (Nivel 10).

#include "directorios.h"
#include <string.h>
#include <time.h>

/*
AUTORES
=======
* Calvache Vidal, Antonio
* Mercadal Forteza, Lluís
* Salvatierra Romero, Diego Jeroni
*/

// Estructura para la caché de directorios (Nivel 9)
struct UltimaEntrada
{
    char camino[TAMNOMBRE * PROFUNDIDAD];
    int p_inodo;
};

// Variables globales estáticas para la caché (una para lectura, otra para escritura)
static struct UltimaEntrada UltimaEntradaLectura;
static struct UltimaEntrada UltimaEntradaEscritura;

// --- FUNCIONES NIVEL 7 (extraer_camino, buscar_entrada, etc.) ---
/**
 * extraer_camino
 * ------
 * Separa el primer componente del camino del resto.
 * Ejemplo: /home/user/docs -> inicial="home", final="/user/docs", tipo='d'
 * * Parámetros:
 * - camino: La ruta completa.
 * - inicial: Buffer para el primer componente.
 * - final: Buffer para el resto de la ruta.
 * - tipo: Buffer para el tipo ('f' o 'd').
 * * Retorno:
 * - Devuelve EXITO (0) si tiene éxito, o ERROR_CAMINO_INCORRECTO si el camino es inválido.
 * * Llama a: strchr(), strncpy(), strcpy()
 */
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo)
{
    if (camino == NULL || inicial == NULL || final == NULL || tipo == NULL)
        return ERROR_CAMINO_INCORRECTO;

    // 1. Ignoramos la primera barra '/'
    if (camino[0] != '/')
        return ERROR_CAMINO_INCORRECTO;

    // Trabajamos con una copia local temporal para buscar el siguiente delimitador
    const char *resto = camino + 1; // Saltamos el primer '/'

    // Buscamos el siguiente separador '/'
    char *separador = strchr(resto, '/');

    if (separador)
    {
        // Caso: Hay más componentes (/home/user)
        // Copiamos lo que hay entre el primer '/' y el segundo '/' en 'inicial'
        int longitud = separador - resto;
        strncpy(inicial, resto, longitud);
        inicial[longitud] = '\0'; // Aseguramos fin de cadena

        // El resto va a 'final'
        strcpy(final, separador);

        // Determinamos el tipo. Si 'final' no está vacío, lo tratamos como directorio
        *tipo = 'd';
    }
    else
    {
        // Caso: Es el último componente (/fichero)
        strcpy(inicial, resto);
        strcpy(final, ""); // No queda nada más
        *tipo = 'f';       // Por defecto 'f', aunque podría ser un dir si fuera mkdir
    }

    return EXITO;
}

/**
 * buscar_entrada
 * ------
 * Función recursiva para resolver rutas (path resolution).
 * Busca una entrada en el directorio indicado por p_inodo_dir. Si no la
 * encuentra y reservar=1, la crea.
 * * Parámetros:
 * - camino_parcial: La ruta que falta por recorrer.
 * - p_inodo_dir: Inodo del directorio donde buscar la entrada actual (inicialmente la raíz).
 * - p_inodo: Puntero donde devolveremos el inodo encontrado (o creado).
 * - p_entrada: Puntero para devolver el índice del directorio padre donde se encontró.
 * - reservar: 1 si queremos crear la entrada final en caso no exista, 0 para solo buscar.
 * - permisos: Permisos a asignar si se crea (solo si reservar==1).
 * * Retorno:
 * - Devuelve EXITO (0) si tiene éxito, o un código de error negativo (ERROR_CAMINO_INCORRECTO, ERROR_PERMISO_LECTURA, etc.) en caso de fallo.
 * * Llama a: bread(), extraer_camino(), leer_inodo(), mi_read_f(), reservar_inodo(), mi_write_f(), liberar_inodo(), buscar_entrada()
 */
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos)
{
    struct inodo inodo_dir;
    char inicial[TAMNOMBRE];
    char final[strlen(camino_parcial) + 1]; // +1 para seguridad
    char tipo;
    int num_entrada_inodo;

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO)
    {
        return FALLO;
    }

    // 1. Caso base: Terminamos la recursión si el camino es la raíz "/"
    if (strcmp(camino_parcial, "/") == 0)
    {
        *p_inodo = SB.posInodoRaiz; // Inodo 0
        *p_entrada = 0;
        return EXITO;
    }

    // 2. Extraer el primer componente del camino
    memset(inicial, 0, TAMNOMBRE);
    memset(final, 0, sizeof(final));
    if (extraer_camino(camino_parcial, inicial, final, &tipo) < 0)
    {
        return ERROR_CAMINO_INCORRECTO;
    }

    #if DEBUGN7
    fprintf(stderr, "[buscar_entrada()→ inicial: %s, final: %s, reservar: %d]\n", inicial, final, reservar);
    #endif

    // 3. Leemos el inodo del directorio actual para buscar en él
    if (leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO)
        return FALLO;

    // Verificamos que tenga permiso de lectura
    if ((inodo_dir.permisos & PERMISO_LECTURA) != PERMISO_LECTURA)
    {
        #if DEBUGN7
        fprintf(stderr, "[buscar_entrada()→ El inodo %d no tiene permisos de lectura]\n", *p_inodo_dir);
        #endif
        return ERROR_PERMISO_LECTURA;
    }

    // Inicializamos buffer de lectura para mayor eficiencia
    // Buffer para leer varias entradas a la vez si quisiéramos optimizar,
    // pero aquí leeremos una a una usando el offset lógico.
    // Número de entradas actuales en el directorio:
    num_entrada_inodo = 0; // Contador de la entrada que estamos leyendo

    struct entrada buffer_entradas[BLOCKSIZE / sizeof(struct entrada)];
    int offset = 0;

    // 4. Búsqueda de la entrada 'inicial' en el directorio actual
    int encontrado = 0;

    // Leemos por bloques completos para optimizar (reducción de accesos a disco)
    while (offset < inodo_dir.tamEnBytesLog && !encontrado)
    {
        // Leemos un bloque de entradas o lo que quede
        int leidos = mi_read_f(*p_inodo_dir, buffer_entradas, offset, BLOCKSIZE);
        if (leidos < 0)
            return FALLO;

        // Iteramos sobre las entradas leídas en el buffer
        int num_entradas_en_buffer = leidos / sizeof(struct entrada);
        for (int i = 0; i < num_entradas_en_buffer; i++)
        {
            if (strcmp(inicial, buffer_entradas[i].nombre) == 0)
            {
                // ¡Encontrado!
                *p_inodo = buffer_entradas[i].ninodo; // Guarda el inodo encontrado
                *p_entrada = num_entrada_inodo;       // Índice global
                encontrado = 1;
                break;
            }
            num_entrada_inodo++;
        }
        offset += leidos;
    }

    // 5. Lógica post-búsqueda
    if (encontrado)
    {
        // Si hemos encontrado la entrada

        // Si el camino final está vacío (final == "") O es solo "/", hemos llegado al objetivo
        if (strcmp(final, "") == 0 || strcmp(final, "/") == 0)
        {
            // Si queríamos reservar y ya existe -> Error (salvo que sea un fichero y queramos sobrescribir,
            // pero el enunciado suele pedir error si ya existe en mkdir).
            if (reservar == 1)
            {
                return ERROR_ENTRADA_YA_EXISTENTE;
            }
            // Si solo buscábamos -> Éxito
            return EXITO;
        }

        // Si queda camino por recorrer (final != "")
        // Recursividad: Ahora el directorio padre será el inodo que acabamos de encontrar
        *p_inodo_dir = *p_inodo;
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    }
    else
    {
        // No encontrado

        // Si queda camino por recorrer (ej: buscando /a/b y no existe 'a'), es un error intermedio
        if (strcmp(final, "") != 0 && strcmp(final, "/") != 0)
        {
            return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
        }

        // Si es el final del camino y queremos crear (reservar == 1)
        if (reservar == 1)
        {
            // 1. Comprobamos que el inodo padre sea un directorio, no un fichero
            if (inodo_dir.tipo == 'f') 
            {
                return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
            }

            // 2. Verificamos permisos de escritura en el directorio padre
            if ((inodo_dir.permisos & 2) != 2)
            {
                return ERROR_PERMISO_ESCRITURA;
            }

            // Reservamos un nuevo inodo para el fichero/directorio nuevo
            // NOTA: Para distinguir si es fichero o directorio usamos el 'tipo' extraído
            // 'd' si había slash al final (extraer_camino lo detecta si el usuario pone /ruta/)
            // o basándonos en el argumento 'permisos' o 'tipo' pasado (que no tenemos explícito en args).
            // Usaremos el argumento 'tipo' para concordancia con el pseudocódigo del enunciado.
            unsigned char tipo_inodo;
            if (tipo == 'd')
            {
                if (strcmp(final, "") == 0 || strcmp(final, "/") == 0)
                {
                    tipo_inodo = 'd';
                }
                else
                {
                    return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
                }
            }
            else
            {
                tipo_inodo = 'f';
            }

            int ninodo_nuevo = reservar_inodo(tipo_inodo, permisos);
            if (ninodo_nuevo == FALLO)
                return FALLO;

            #if DEBUGN7
            fprintf(stderr, "[buscar_entrada()→ reservado inodo %d tipo %c con permisos %d para %s]\n", ninodo_nuevo, tipo_inodo, permisos, inicial);
            #endif

            // Preparamos la nueva entrada
            struct entrada nueva_entrada;
            strncpy(nueva_entrada.nombre, inicial, TAMNOMBRE);
            nueva_entrada.ninodo = ninodo_nuevo;

            // Escribimos la entrada al final del directorio padre
            if (mi_write_f(*p_inodo_dir, &nueva_entrada, inodo_dir.tamEnBytesLog, sizeof(struct entrada)) == FALLO)
            {
                // Si falla, liberamos el inodo que reservamos para no dejar basura
                liberar_inodo(ninodo_nuevo);
                return FALLO;
            }

            #if DEBUGN7
            fprintf(stderr, "[buscar_entrada()→ creada entrada: %s, %d]\n", inicial, ninodo_nuevo);
            #endif

            // Devolvemos el inodo creado
            *p_inodo = ninodo_nuevo;
            *p_entrada = num_entrada_inodo; // El índice es el último

            // printf("[DEBUG] Creada entrada '%s' con inodo %d en directorio %d\n", inicial, ninodo_nuevo, *p_inodo_dir);
            return EXITO;
        }

        // Si no queríamos reservar y no existe
        return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
    }
}

/**
 * mostrar_error_buscar_entrada
 * ------
 * Imprime el mensaje correspondiente al código de error.
 * * Parámetros:
 * - error: Código de error devuelto por buscar_entrada() u otras operaciones.
 * * Retorno:
 * - Ninguno (void).
 * * Llama a: fprintf()
 */
void mostrar_error_buscar_entrada(int error)
{
    switch (error)
    {
    case ERROR_CAMINO_INCORRECTO:
        fprintf(stderr, RED "Error: Camino incorrecto.\n" RESET);
        break;
    case ERROR_PERMISO_LECTURA:
        fprintf(stderr, RED "Error: Permiso denegado de lectura.\n" RESET);
        break;
    case ERROR_NO_EXISTE_ENTRADA_CONSULTA:
        fprintf(stderr, RED "Error: No existe el archivo o directorio.\n" RESET);
        break;
    case ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO:
        fprintf(stderr, RED "Error: No existe algún directorio intermedio.\n" RESET);
        break;
    case ERROR_PERMISO_ESCRITURA:
        fprintf(stderr, RED "Error: Permiso denegado de escritura.\n" RESET);
        break;
    case ERROR_ENTRADA_YA_EXISTENTE:
        fprintf(stderr, RED "Error: El archivo ya existe.\n" RESET);
        break;
    case ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO:
        fprintf(stderr, RED "Error: No es un directorio.\n" RESET);
        break;
    case ERROR_NO_ES_DIRECTORIO:
        fprintf(stderr, RED "Error: No es un directorio.\n" RESET);
        break;
    case ERROR_DIRECTORIO_NO_VACIO:
        fprintf(stderr, RED "Error: El directorio no esta vacio.\n" RESET);
        break;
    case FALLO:
        fprintf(stderr, RED "Error: Fallo critico de E/S en el dispositivo.\n" RESET);
        break;
    }
}

// ==============================================================================
// FUNCIONES NIVEL 8
// ==============================================================================

/**
 * mi_creat
 * ------
 * Crea un fichero o directorio en la ruta especificada.
 * Si el camino termina en '/', se asume directorio.
 * * Parámetros:
 * - camino: Ruta absoluta de la nueva entrada.
 * - permisos: Permisos que se le asignarán a la entrada (0-7).
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o un código de error negativo.
 * * Llama a: buscar_entrada()
 */
int mi_creat(const char *camino, unsigned char permisos)
{
    mi_waitSem();

    unsigned int p_inodo_dir = 0; // Raíz
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;

    // Llamamos a buscar_entrada con reservar = 1 para crear
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);

    if (error < 0)
    {
        mi_signalSem();
        return error;
    }
    mi_signalSem();
    return EXITO;
}

/**
 * mi_dir
 * ------
 * Pone el contenido del directorio en un buffer.
 * Soporta formato simple (solo nombres) y extendido (detalles, estilo 'ls -l').
 * También permite mostrar la información de un único fichero si la ruta apunta a uno.
 * * Parámetros:
 * - camino: Ruta del directorio (o fichero) a listar.
 * - buffer: Buffer donde se almacenará la cadena de texto con el resultado.
 * - tipo: Flag que indica si se espera un directorio ('d') o un fichero ('f').
 * - extendido: Flag (1 o 0) que indica si se requiere formato detallado (1) o simple (0).
 * * Retorno:
 * - Devuelve el número de entradas leídas (1 si es fichero), o un código de error negativo.
 * * Llama a: buscar_entrada(), leer_inodo(), strrchr(), strcpy(), localtime(), sprintf(), mi_read_f(), snprintf(), strcat()
 */
int mi_dir(const char *camino, char *buffer, char tipo, int extendido)
{
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;
    struct inodo inodo;

    // 1. Buscamos la entrada (solo consulta, sin reservar)
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, PERMISO_LECTURA);
    if (error < 0)
        return error;

    // 2. Leemos el inodo asociado a la entrada encontrada
    if (leer_inodo(p_inodo, &inodo) == FALLO)
        return FALLO;

    // 3. Validamos el tipo: si esperábamos directorio ('d') pero es un fichero, lanzamos error
    if (tipo == 'd' && inodo.tipo != 'd')
    {
        return ERROR_NO_ES_DIRECTORIO;
    }

    strcpy(buffer, "");

    // CASO A: Es un fichero -> Mostramos su información
    if (inodo.tipo == 'f')
    {
        char tmp[TAMFILA];
        char nombre[TAMNOMBRE];
        
        // Extraer solo el nombre del fichero de la ruta completa
        const char *ultimo_slash = strrchr(camino, '/');
        if (ultimo_slash != NULL) {
            strcpy(nombre, ultimo_slash + 1);
        } else {
            strcpy(nombre, camino);
        }

        if (extendido) {
            char fecha[50];
            struct tm *tm;
            tm = localtime(&inodo.mtime);
            sprintf(fecha, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

            sprintf(tmp, "%c\t%c%c%c\t%s\t%d\t\x1b[32m%s\x1b[0m\n",
                    inodo.tipo,
                    (inodo.permisos & PERMISO_LECTURA) ? 'r' : '-',
                    (inodo.permisos & PERMISO_ESCRITURA) ? 'w' : '-',
                    (inodo.permisos & PERMISO_EJECUCION) ? 'x' : '-',
                    fecha,
                    inodo.tamEnBytesLog,
                    nombre);
        } else {
            sprintf(tmp, "\x1b[32m%s\x1b[0m\n", nombre);
        }
        strcat(buffer, tmp);
        return 1;
    }

    // CASO B: Es un directorio -> Iteramos sus entradas
    if (inodo.tipo == 'd')
    {
        struct entrada buffer_entradas[BLOCKSIZE / sizeof(struct entrada)];
        struct inodo inodo_ent;
        int offset = 0;
        int entradas_leidas = 0;
        int num_entradas = 0;

        while (offset < inodo.tamEnBytesLog)
        {
            int leidos = mi_read_f(p_inodo, buffer_entradas, offset, BLOCKSIZE);
            if (leidos < 0)
                return leidos;

            num_entradas = leidos / sizeof(struct entrada);

            for (int i = 0; i < num_entradas; i++)
            {
                leer_inodo(buffer_entradas[i].ninodo, &inodo_ent);

                char tmp[TAMFILA];
                char color[10] = "";
                if (inodo_ent.tipo == 'd')
                    strcpy(color, "\x1b[34m"); // Azul
                else
                    strcpy(color, "\x1b[32m"); // Verde

                if (extendido) {
                    char fecha[50];
                    struct tm *tm;
                    tm = localtime(&inodo_ent.mtime);
                    sprintf(fecha, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

                    snprintf(tmp, sizeof(tmp), "%c\t%c%c%c\t%s\t%d\t%s%s\x1b[0m\n",
                             inodo_ent.tipo,
                             (inodo_ent.permisos & PERMISO_LECTURA) ? 'r' : '-',
                             (inodo_ent.permisos & PERMISO_ESCRITURA) ? 'w' : '-',
                             (inodo_ent.permisos & PERMISO_EJECUCION) ? 'x' : '-',
                             fecha,
                             inodo_ent.tamEnBytesLog,
                             color,
                             buffer_entradas[i].nombre);
                } else {
                    // Formato simple: solo el nombre con un tabulador
                    snprintf(tmp, sizeof(tmp), "%s%s\x1b[0m\t", color, buffer_entradas[i].nombre);
                }

                if (strlen(buffer) + strlen(tmp) < TAMBUFFER)
                {
                    strcat(buffer, tmp);
                    entradas_leidas++;
                }
            }
            offset += leidos;
        }
        
        // Si es formato simple, añadimos un salto de línea al final para que quede limpio
        if (!extendido && entradas_leidas > 0) {
            strcat(buffer, "\n");
        }
        
        return entradas_leidas;
    }

    return EXITO;
}

/**
 * mi_chmod
 * ------
 * Cambia los permisos de un fichero/directorio.
 * * Parámetros:
 * - camino: Ruta del fichero o directorio al que se le cambiarán los permisos.
 * - permisos: Nuevos permisos a asignar (0-7).
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o un código de error negativo.
 * * Llama a: buscar_entrada(), mi_chmod_f()
 */
int mi_chmod(const char *camino, unsigned char permisos)
{
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;

    // Buscamos la entrada (no reservamos, solo consultamos)
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (error < 0)
        return error;

    // Llamamos a la función de nivel inferior
    return mi_chmod_f(p_inodo, permisos);
}

/**
 * mi_stat
 * ------
 * Obtiene la información del inodo de un fichero/directorio.
 * * Parámetros:
 * - camino: Ruta de la entrada a examinar.
 * - p_stat: Puntero a la estructura STAT donde se guardarán los metadatos.
 * * Retorno:
 * - Devuelve el número del inodo de la entrada o un código de error negativo.
 * * Llama a: buscar_entrada(), mi_stat_f()
 */
int mi_stat(const char *camino, struct STAT *p_stat)
{
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;

    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (error < 0)
        return error;

    // Llamamos a la función de nivel inferior
    if (mi_stat_f(p_inodo, p_stat) == FALLO)
        return FALLO;

    // Devolvemos el número de inodo (truco: lo ponemos en una variable global o lo imprimirá el main)
    // El enunciado dice "Mostrar también el nº de inodo", eso lo hará mi_stat.c
    // Pero necesitamos devolverlo. Podemos usar un campo auxiliar o devolverlo en return si EXITO=0.
    // Como mi_stat_f devuelve EXITO/FALLO, devolveremos el ninodo si éxito.
    return p_inodo;
}

// ==============================================================================
// FUNCIONES NIVEL 9
// ==============================================================================

/**
 * mi_write
 * ------
 * Escribe contenido en un fichero indicado por su ruta.
 * Utiliza una caché para no llamar a buscar_entrada si escribimos varias veces en el mismo fichero.
 * * Parámetros:
 * - camino: Ruta del fichero donde se va a escribir.
 * - buf: Buffer de memoria con los datos a escribir.
 * - offset: Posición lógica a partir de la cual comenzar la escritura.
 * - nbytes: Cantidad de bytes a escribir.
 * * Retorno:
 * - Devuelve la cantidad de bytes escritos o un código de error negativo.
 * * Llama a: strcmp(), buscar_entrada(), strcpy(), mi_write_f()
 */
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes)
{
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;

    // 1. Comprobar caché de escritura
    if (strcmp(UltimaEntradaEscritura.camino, camino) == 0)
    {
        // Acierto en caché (escribimos en el mismo fichero que la última vez)
        p_inodo = UltimaEntradaEscritura.p_inodo;
        #if DEBUGN9
        fprintf(stderr, "[mi_write() -> Utilizamos la caché de escritura]\n");
        #endif
    }
    else
    {
        // Fallo en caché: buscamos la entrada
        error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (error < 0)
            return error;

        // Actualizamos la caché
        strcpy(UltimaEntradaEscritura.camino, camino);
        UltimaEntradaEscritura.p_inodo = p_inodo;
        #if DEBUGN9
        fprintf(stderr, "[mi_write() -> Actualizamos la caché de escritura]\n");
        #endif
    }

    // 2. Escribir usando la función de nivel inferior (ficheros.c)
    // mi_write_f verificará los permisos de escritura del inodo
    int bytes_escritos = mi_write_f(p_inodo, buf, offset, nbytes);

    return bytes_escritos;
}

/**
 * mi_read
 * ------
 * Lee contenido de un fichero indicado por su ruta.
 * Utiliza caché de lectura.
 * * Parámetros:
 * - camino: Ruta del fichero a leer.
 * - buf: Buffer de memoria donde se guardarán los datos.
 * - offset: Posición a partir de la cual comenzar a leer.
 * - nbytes: Cantidad de bytes a leer.
 * * Retorno:
 * - Devuelve la cantidad de bytes leídos o un código de error negativo.
 * * Llama a: strcmp(), buscar_entrada(), strcpy(), mi_read_f()
 */
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes)
{
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;

    // 1. Comprobar caché de lectura
    if (strcmp(UltimaEntradaLectura.camino, camino) == 0)
    {
        // Acierto en caché
        p_inodo = UltimaEntradaLectura.p_inodo;
        #if DEBUGN9
        fprintf(stderr, "[mi_read() -> Utilizamos la caché de lectura]\n");
        #endif
    }
    else
    {
        // Fallo en caché: buscamos
        error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (error < 0)
            return error;

        // Actualizamos la caché
        strcpy(UltimaEntradaLectura.camino, camino);
        UltimaEntradaLectura.p_inodo = p_inodo;
        #if DEBUGN9
        fprintf(stderr, "[mi_read() -> Actualizamos la caché de lectura]\n");
        #endif
    }

    // 2. Leer usando la función de nivel inferior
    int bytes_leidos = mi_read_f(p_inodo, buf, offset, nbytes);

    return bytes_leidos;
}

// ==============================================================================
// FUNCIONES NIVEL 10
// ==============================================================================

/**
 * mi_link
 * ------
 * Crea un enlace físico (hard link) de camino2 apuntando al inodo de camino1.
 * * Parámetros:
 * - camino1: Ruta del fichero original ya existente.
 * - camino2: Ruta del nuevo enlace a crear.
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o un código de error negativo.
 * * Llama a: buscar_entrada(), leer_inodo(), mi_read_f(), mi_write_f(), liberar_inodo(), escribir_inodo()
 */
int mi_link(const char *camino1, const char *camino2)
{
    mi_waitSem();

    unsigned int p_inodo_dir1 = 0, p_inodo1 = 0, p_entrada1 = 0;
    unsigned int p_inodo_dir2 = 0, p_inodo2 = 0, p_entrada2 = 0;
    int error;
    struct inodo inodo1;

    // 1. Verificar existencia de camino1 (origen)
    error = buscar_entrada(camino1, &p_inodo_dir1, &p_inodo1, &p_entrada1, 0, 0);
    if (error < 0) {
        mi_signalSem();
        return error;
    }

    // 2. Leer inodo1 para verificar que es fichero y tenemos permisos
    if (leer_inodo(p_inodo1, &inodo1) == FALLO)
    {
        mi_signalSem();
        return FALLO;
    }

    if (inodo1.tipo == 'd')
    {
        mi_signalSem();
        return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO; // No se permiten enlaces a directorios (para evitar ciclos)
    }
    if ((inodo1.permisos & PERMISO_LECTURA) != PERMISO_LECTURA)
    { // Permiso lectura
        mi_signalSem();
        return ERROR_PERMISO_LECTURA;
    }

    // 3. Crear entrada para camino2 (destino)
    // Usamos buscar_entrada con reservar=1. Esto creará un inodo NUEVO automáticamente.
    error = buscar_entrada(camino2, &p_inodo_dir2, &p_inodo2, &p_entrada2, 1, 6);
    if (error < 0) {
        mi_signalSem();
        return error; // Si ya existe, devolverá error
    }

    // 4. "Truco" del enlace físico:
    // buscar_entrada ha creado una entrada en el directorio padre de camino2 que apunta a p_inodo2 (nuevo).
    // Queremos que apunte a p_inodo1.

    struct entrada entrada;
    // Leemos la entrada que acabamos de crear en el directorio padre (p_inodo_dir2)
    if (mi_read_f(p_inodo_dir2, &entrada, p_entrada2 * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
            mi_signalSem();
            return FALLO;
    }

    // Modificamos el puntero al inodo
    entrada.ninodo = p_inodo1;

    // Escribimos la entrada modificada
    if (mi_write_f(p_inodo_dir2, &entrada, p_entrada2 * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
        mi_signalSem();
        return FALLO;
    }

    // 5. Liberamos el inodo2 que se creó innecesariamente
    if (liberar_inodo(p_inodo2) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    // 6. Actualizamos el inodo1 (el original)
    inodo1.nlinks++;
    inodo1.ctime = time(NULL);
    if (escribir_inodo(p_inodo1, &inodo1) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    mi_signalSem();
    return EXITO;
}

/**
 * mi_unlink
 * ------
 * Borra la entrada de directorio especificada. Si es el último enlace, libera el inodo.
 * * Parámetros:
 * - camino: Ruta del fichero o directorio a eliminar.
 * * Retorno:
 * - Devuelve EXITO (0) si finaliza correctamente, o un código de error negativo.
 * * Llama a: buscar_entrada(), leer_inodo(), mi_read_f(), mi_write_f(), mi_truncar_f(), liberar_inodo(), escribir_inodo()
 */
int mi_unlink(const char *camino)
{
    mi_waitSem();
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int error;
    struct inodo inodo_dir, inodo_borrar;
    struct entrada entrada;

    // 1. Buscar la entrada a borrar
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (error < 0) {
        mi_signalSem();
        return error;
    }

    // 2. Verificaciones sobre lo que vamos a borrar
    if (leer_inodo(p_inodo, &inodo_borrar) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    if (inodo_borrar.tipo == 'd' && inodo_borrar.tamEnBytesLog > 0)
    {
        // Es un directorio y no está vacío
        mi_signalSem();
        return ERROR_DIRECTORIO_NO_VACIO;
    }

    // 3. Eliminar la entrada del directorio padre
    if (leer_inodo(p_inodo_dir, &inodo_dir) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    int num_entradas = inodo_dir.tamEnBytesLog / sizeof(struct entrada);

    // Si no es la última entrada, tenemos que compactar el directorio
    // Copiamos la última entrada en la posición de la que borramos
    if (p_entrada != num_entradas - 1)
    {
        // Leer la última entrada
        if (mi_read_f(p_inodo_dir, &entrada, (num_entradas - 1) * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
            mi_signalSem();
            return FALLO;
        }
        
        // Escribirla en la posición hueca (p_entrada)
        if (mi_write_f(p_inodo_dir, &entrada, p_entrada * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
            mi_signalSem();
            return FALLO;
        }
    }

    // Truncamos el directorio padre para eliminar la última posición
    if (mi_truncar_f(p_inodo_dir, (num_entradas - 1) * sizeof(struct entrada)) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    // 4. Actualizar el inodo borrado (decrementar nlinks)
    inodo_borrar.nlinks--;

    if (inodo_borrar.nlinks == 0)
    {
        // Si no quedan enlaces, liberar inodo y sus datos
        if (liberar_inodo(p_inodo) == FALLO) {
            mi_signalSem(); 
            return FALLO;
        }
    }
    else
    {
        // Si quedan enlaces, solo actualizamos ctime
        inodo_borrar.ctime = time(NULL);
        if (escribir_inodo(p_inodo, &inodo_borrar) == FALLO) {
            mi_signalSem();
            return FALLO;
        }
    }

    // LIMPIEZA DE CACHÉ
    // Vaciamos la memoria caché si coincide con el fichero borrado
    if (strcmp(UltimaEntradaLectura.camino, camino) == 0) {
        memset(UltimaEntradaLectura.camino, 0, sizeof(UltimaEntradaLectura.camino));
        UltimaEntradaLectura.p_inodo = -1; 
    }
    if (strcmp(UltimaEntradaEscritura.camino, camino) == 0) {
        memset(UltimaEntradaEscritura.camino, 0, sizeof(UltimaEntradaEscritura.camino));
        UltimaEntradaEscritura.p_inodo = -1;
    }

    mi_signalSem();
    return EXITO;
}