// directorios.c
// Descripción: Implementación de la capa de directorios (Nivel 10).

#include "directorios.h"
#include <string.h>
#include <time.h>

// Estructura para la caché de directorios (Nivel 9)
struct UltimaEntrada {
    char camino[TAMNOMBRE * PROFUNDIDAD];
    int p_inodo;
};

// Variables globales estáticas para la caché (una para lectura, otra para escritura)
static struct UltimaEntrada UltimaEntradaLectura;
static struct UltimaEntrada UltimaEntradaEscritura;

// --- FUNCIONES NIVEL 7 (extraer_camino, buscar_entrada, etc.) ---
/**
 * extraer_camino
 * Separa el primer componente del camino del resto.
 * Ejemplo: /home/user/docs -> inicial="home", final="/user/docs", tipo='d'
 */
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo)
{
    if (camino == NULL || inicial == NULL || final == NULL || tipo == NULL)
        return -1;

    // 1. Ignoramos la primera barra '/'
    if (camino[0] != '/')
        return -1;

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
 * Función recursiva para resolver rutas (path resolution).
 * - camino_parcial: La ruta que falta por recorrer.
 * - p_inodo_dir: Inodo del directorio padre actual.
 * - p_inodo: Puntero donde devolveremos el inodo encontrado (o creado).
 * - p_entrada: Puntero para devolver el índice de la entrada dentro del directorio.
 * - reservar: 1 si queremos crear la entrada final, 0 para solo buscar.
 * - permisos: Permisos a asignar si se crea (solo si reservar==1).
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
        if (reservar == 1)
        {
            return ERROR_ENTRADA_YA_EXISTENTE; // No se puede crear la raíz
        }
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

    // printf("[DEBUG] Buscando '%s' dentro del inodo %d. Resta: '%s'\n", inicial, *p_inodo_dir, final);

    // 3. Leemos el inodo del directorio actual para buscar en él
    if (leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO)
        return FALLO;

    // Verificamos que sea un directorio
    if (inodo_dir.tipo != 'd')
    {
        return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_FICHERO;
    }

    // Verificamos permisos: lectura para buscar, escritura para reservar
    if (reservar == 1)
    {
        if ((inodo_dir.permisos & 2) != 2)
        {
            return ERROR_PERMISO_ESCRITURA;
        }
    }
    else
    {
        if ((inodo_dir.permisos & 4) != 4)
        {
            return ERROR_PERMISO_LECTURA;
        }
    }

    // Inicializamos buffer de lectura para mayor eficiencia
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
                *p_inodo = buffer_entradas[i].ninodo;
                *p_entrada = num_entrada_inodo; // Índice global
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
            // Si queríamos reservar y ya existe -> Error
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
            // Verificamos permisos de escritura en el directorio padre
            if ((inodo_dir.permisos & 2) != 2)
            {
                return ERROR_PERMISO_ESCRITURA;
            }

            // Reservamos un nuevo inodo para el fichero/directorio nuevo
            // Usamos el tipo extraído por extraer_camino: 'd' si acaba en '/', 'f' si no
            unsigned char tipo_inodo = tipo;

            int ninodo_nuevo = reservar_inodo(tipo_inodo, permisos);
            if (ninodo_nuevo == FALLO)
                return FALLO;

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

            // Devolvemos el inodo creado
            *p_inodo = ninodo_nuevo;
            *p_entrada = num_entrada_inodo; // El índice es el último

            // printf("[DEBUG] Creada entrada '%s' con inodo %d en directorio %d\n", inicial, ninodo_nuevo, *p_inodo_dir);
            return EXITO;
        }

        // Si no queríamos reservar y no existe
        return ERROR_NO_EXISTE_ENTRADA;
    }
}

/**
 * mostrar_error_buscar_entrada
 * Imprime el mensaje correspondiente al código de error.
 */
void mostrar_error_buscar_entrada(int error)
{
    switch (error)
    {
    case ERROR_CAMINO_INCORRECTO:
        fprintf(stderr, "Error: Camino incorrecto.\n");
        break;
    case ERROR_PERMISO_LECTURA:
        fprintf(stderr, "Error: Permiso denegado de lectura.\n");
        break;
    case ERROR_NO_EXISTE_ENTRADA:
        fprintf(stderr, "Error: No existe el archivo o directorio.\n");
        break;
    case ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO:
        fprintf(stderr, "Error: No existe algún directorio intermedio.\n");
        break;
    case ERROR_PERMISO_ESCRITURA:
        fprintf(stderr, "Error: Permiso denegado de escritura.\n");
        break;
    case ERROR_ENTRADA_YA_EXISTENTE:
        fprintf(stderr, "Error: El archivo ya existe.\n");
        break;
    case ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_FICHERO:
        fprintf(stderr, "Error: No es un directorio.\n");
        break;
    }
}

// ==============================================================================
// FUNCIONES NIVEL 8
// ==============================================================================

/**
 * mi_creat
 * Crea un fichero o directorio en la ruta especificada.
 * Si el camino termina en '/', se asume directorio.
 */
int mi_creat(const char *camino, unsigned char permisos) {
    unsigned int p_inodo_dir = 0; // Raíz
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;

    // Llamamos a buscar_entrada con reservar = 1 para crear
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);
    
    if (error < 0) {
        return error;
    }
    return EXITO;
}

/**
 * mi_dir
 * Pone el contenido del directorio en un buffer.
 * Soporta listar un solo fichero si 'camino' apunta a un fichero.
 * 'tipo' es un flag que indica el formato: 'd' (directorio/simple) o 'f' (fichero/detallado?)
 * NOTA: Para este nivel, adaptamos la firma según el PDF para soportar ls -l
 * Parámetro 'tipo': Lo usaremos para filtrar o validar, pero el PDF sugiere 
 * usarlo para distinguir visualización.
 */
int mi_dir(const char *camino, char *buffer, char tipo) {
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;
    struct inodo inodo;
    
    // 1. Buscamos la entrada
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4); // Permisos lectura
    if (error < 0) return error;

    // 2. Leemos el inodo
    if (leer_inodo(p_inodo, &inodo) == FALLO) return FALLO;

    // 3. Comprobación de consistencia tipo camino vs tipo inodo
    // Si el usuario puso '/' al final (tipo 'd') pero es fichero, error.
    if (tipo == 'd' && inodo.tipo != 'd') {
        return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_FICHERO; // Reusamos este error o creamos uno
    }

    // Inicializamos buffer
    strcpy(buffer, "");

    // CASO A: Es un fichero -> Mostramos su información
    if (inodo.tipo == 'f') {
        char tmp[TAMFILA];
        char fecha[50];
        struct tm *tm;

        tm = localtime(&inodo.mtime);
        sprintf(fecha, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

        // Formato: Tipo Permisos mTime Tamaño Nombre
        sprintf(tmp, "%c\t%c%c%c\t%s\t%d\t%s\n",
                inodo.tipo,
                (inodo.permisos & 4) ? 'r' : '-',
                (inodo.permisos & 2) ? 'w' : '-',
                (inodo.permisos & 1) ? 'x' : '-',
                fecha,
                inodo.tamEnBytesLog,
                camino); // Usamos el camino completo o extraemos nombre
        strcat(buffer, tmp);
        return 1;
    }

    // CASO B: Es un directorio -> Iteramos sus entradas
    if (inodo.tipo == 'd') {
        struct entrada buffer_entradas[BLOCKSIZE / sizeof(struct entrada)];
        struct inodo inodo_ent;
        int offset = 0;
        int entradas_leidas = 0;
        int num_entradas = 0;
        
        // Leemos entradas del directorio
        while (offset < inodo.tamEnBytesLog) {
            int leidos = mi_read_f(p_inodo, buffer_entradas, offset, BLOCKSIZE);
            if (leidos < 0) return leidos; // Error lectura
            
            num_entradas = leidos / sizeof(struct entrada);
            
            for (int i = 0; i < num_entradas; i++) {
                // Leemos el inodo de la entrada para obtener sus detalles
                leer_inodo(buffer_entradas[i].ninodo, &inodo_ent);
                
                char tmp[TAMFILA];
                char fecha[50];
                struct tm *tm;

                tm = localtime(&inodo_ent.mtime);
                sprintf(fecha, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

                // Colores simples: Directorios en Azul, Ficheros en Verde
                char color[10] = "";
                if(inodo_ent.tipo == 'd') strcpy(color, "\x1b[34m"); // Azul
                else strcpy(color, "\x1b[32m"); // Verde

                snprintf(tmp, sizeof(tmp), "%c\t%c%c%c\t%s\t%d\t%s%s\x1b[0m\n",
                    inodo_ent.tipo,
                    (inodo_ent.permisos & 4) ? 'r' : '-',
                    (inodo_ent.permisos & 2) ? 'w' : '-',
                    (inodo_ent.permisos & 1) ? 'x' : '-',
                    fecha,
                    inodo_ent.tamEnBytesLog,
                    color,
                    buffer_entradas[i].nombre);

                if (strlen(buffer) + strlen(tmp) < TAMBUFFER) {
                    strcat(buffer, tmp);
                    entradas_leidas++;
                }
            }
            offset += leidos;
        }
        return entradas_leidas;
    }

    return EXITO;
}

/**
 * mi_chmod
 * Cambia los permisos de un fichero/directorio.
 */
int mi_chmod(const char *camino, unsigned char permisos) {
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;

    // Buscamos la entrada (no reservamos, solo consultamos)
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (error < 0) return error;

    // Llamamos a la función de nivel inferior
    return mi_chmod_f(p_inodo, permisos);
}

/**
 * mi_stat
 * Obtiene la información del inodo de un fichero/directorio.
 */
int mi_stat(const char *camino, struct STAT *p_stat) {
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;

    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (error < 0) return error;

    // Llamamos a la función de nivel inferior
    if (mi_stat_f(p_inodo, p_stat) == FALLO) return FALLO;
    
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
 * Escribe contenido en un fichero indicado por su ruta.
 * Utiliza una caché para no llamar a buscar_entrada si escribimos varias veces en el mismo fichero.
 */
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;

    // 1. Comprobar caché de escritura
    if (strcmp(UltimaEntradaEscritura.camino, camino) == 0) {
        // Acierto en caché (escribimos en el mismo fichero que la última vez)
        p_inodo = UltimaEntradaEscritura.p_inodo;
        // Opcional: Debug visual
        // fprintf(stderr, "[mi_write() -> Utilizamos la caché de escritura]\n");
    } else {
        // Fallo en caché: buscamos la entrada
        error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (error < 0) return error;

        // Actualizamos la caché
        strcpy(UltimaEntradaEscritura.camino, camino);
        UltimaEntradaEscritura.p_inodo = p_inodo;
        // fprintf(stderr, "[mi_write() -> Actualizamos la caché de escritura]\n");
    }

    // 2. Verificar permisos de escritura del inodo antes de escribir
    struct inodo inodo;
    if (leer_inodo(p_inodo, &inodo) == FALLO) return FALLO;
    if ((inodo.permisos & 2) != 2) {
        return ERROR_PERMISO_ESCRITURA;
    }

    // 3. Escribir usando la función de nivel inferior (ficheros.c)
    int bytes_escritos = mi_write_f(p_inodo, buf, offset, nbytes);
    
    return bytes_escritos;
}

/**
 * mi_read
 * Lee contenido de un fichero indicado por su ruta.
 * Utiliza caché de lectura.
 */
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;

    // 1. Comprobar caché de lectura
    if (strcmp(UltimaEntradaLectura.camino, camino) == 0) {
        // Acierto en caché
        p_inodo = UltimaEntradaLectura.p_inodo;
        // fprintf(stderr, "[mi_read() -> Utilizamos la caché de lectura]\n");
    } else {
        // Fallo en caché: buscamos
        error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (error < 0) return error;

        // Actualizamos la caché
        strcpy(UltimaEntradaLectura.camino, camino);
        UltimaEntradaLectura.p_inodo = p_inodo;
        // fprintf(stderr, "[mi_read() -> Actualizamos la caché de lectura]\n");
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
 * Crea un enlace físico (hard link) de camino2 apuntando al inodo de camino1.
 * No reserva inodos temporales: añade la entrada directamente al directorio padre.
 */
int mi_link(const char *camino1, const char *camino2) {
    unsigned int p_inodo_dir1 = 0, p_inodo1 = 0, p_entrada1 = 0;
    unsigned int p_inodo_dir2 = 0, p_inodo2 = 0, p_entrada2 = 0;
    int error;
    struct inodo inodo1, inodo_dir2;

    // 1. Verificar existencia de camino1 (origen)
    error = buscar_entrada(camino1, &p_inodo_dir1, &p_inodo1, &p_entrada1, 0, 0);
    if (error < 0) return error;

    // 2. Leer inodo1 para verificar que es fichero y tenemos permisos
    if (leer_inodo(p_inodo1, &inodo1) == FALLO) return FALLO;

    if (inodo1.tipo == 'd') {
        return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_FICHERO; // No se permiten enlaces a directorios (para evitar ciclos)
    }
    if ((inodo1.permisos & 4) != 4) { // Permiso lectura
        return ERROR_PERMISO_LECTURA;
    }

    // 3. Verificar que camino2 NO existe (buscar_entrada con reservar=0)
    //    Si existe, devolverá >= 0; si no, ERROR_NO_EXISTE_ENTRADA.
    //    En este último caso, p_inodo_dir2 queda apuntando al directorio padre.
    error = buscar_entrada(camino2, &p_inodo_dir2, &p_inodo2, &p_entrada2, 0, 0);
    if (error >= 0) {
        return ERROR_ENTRADA_YA_EXISTENTE;
    }
    if (error != ERROR_NO_EXISTE_ENTRADA) {
        return error; // Error real (camino incorrecto, dir intermedio no existe, etc.)
    }

    // 4. Verificar permisos de escritura en el directorio padre de camino2
    if (leer_inodo(p_inodo_dir2, &inodo_dir2) == FALLO) return FALLO;
    if ((inodo_dir2.permisos & 2) != 2) {
        return ERROR_PERMISO_ESCRITURA;
    }

    // 5. Extraer el nombre del fichero (último componente de camino2)
    char nombre_entrada[TAMNOMBRE];
    const char *ultima_barra = strrchr(camino2, '/');
    if (ultima_barra == NULL) {
        return ERROR_CAMINO_INCORRECTO;
    }
    strncpy(nombre_entrada, ultima_barra + 1, TAMNOMBRE - 1);
    nombre_entrada[TAMNOMBRE - 1] = '\0';

    // 6. Crear la entrada apuntando directamente al inodo origen (sin reservar inodo nuevo)
    struct entrada nueva_entrada;
    memset(nueva_entrada.nombre, 0, TAMNOMBRE);
    strncpy(nueva_entrada.nombre, nombre_entrada, TAMNOMBRE - 1);
    nueva_entrada.ninodo = p_inodo1;

    if (mi_write_f(p_inodo_dir2, &nueva_entrada, inodo_dir2.tamEnBytesLog, sizeof(struct entrada)) < 0) {
        return FALLO;
    }

    // 7. Actualizar nlinks del inodo origen
    inodo1.nlinks++;
    inodo1.ctime = time(NULL);
    if (escribir_inodo(p_inodo1, &inodo1) == FALLO) return FALLO;

    return EXITO;
}

/**
 * mi_unlink
 * Borra la entrada de directorio especificada. Si es el último enlace, libera el inodo.
 */
int mi_unlink(const char *camino) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int error;
    struct inodo inodo_dir, inodo_borrar;
    struct entrada entrada;

    // 1. Buscar la entrada a borrar
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (error < 0) return error;

    // 2. Verificaciones sobre lo que vamos a borrar
    if (leer_inodo(p_inodo, &inodo_borrar) == FALLO) return FALLO;

    if (inodo_borrar.tipo == 'd' && inodo_borrar.tamEnBytesLog > 0) {
        // Es un directorio y no está vacío
        return ERROR_ENTRADA_YA_EXISTENTE; // Reusamos código o definimos ERROR_DIRECTORIO_NO_VACIO
    }

    // 3. Eliminar la entrada del directorio padre
    if (leer_inodo(p_inodo_dir, &inodo_dir) == FALLO) return FALLO;

    int num_entradas = inodo_dir.tamEnBytesLog / sizeof(struct entrada);

    // Si no es la última entrada, tenemos que compactar el directorio
    // Copiamos la última entrada en la posición de la que borramos
    if (p_entrada != num_entradas - 1) {
        // Leer la última entrada
        if (mi_read_f(p_inodo_dir, &entrada, (num_entradas - 1) * sizeof(struct entrada), sizeof(struct entrada)) < 0) return FALLO;
        
        // Escribirla en la posición hueca (p_entrada)
        if (mi_write_f(p_inodo_dir, &entrada, p_entrada * sizeof(struct entrada), sizeof(struct entrada)) < 0) return FALLO;
    }

    // Truncamos el directorio padre para eliminar la última posición
    if (mi_truncar_f(p_inodo_dir, (num_entradas - 1) * sizeof(struct entrada)) == FALLO) return FALLO;

    // 4. Actualizar el inodo borrado (decrementar nlinks)
    inodo_borrar.nlinks--;

    if (inodo_borrar.nlinks == 0) {
        // Si no quedan enlaces, liberar inodo y sus datos
        if (liberar_inodo(p_inodo) == FALLO) return FALLO;
    } else {
        // Si quedan enlaces, solo actualizamos ctime
        inodo_borrar.ctime = time(NULL);
        if (escribir_inodo(p_inodo, &inodo_borrar) == FALLO) return FALLO;
    }

    return EXITO;
}