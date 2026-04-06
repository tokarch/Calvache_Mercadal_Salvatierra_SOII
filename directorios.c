// directorios.c
// Descripción: Implementación de la capa de directorios (Nivel 7).

#include "directorios.h"
#include <string.h>

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
    struct entrada entrada_leer;
    struct inodo inodo_dir;
    char inicial[TAMNOMBRE];
    char final[strlen(camino_parcial) + 1]; // +1 para seguridad
    char tipo;
    int cant_entradas_inodo, num_entrada_inodo;

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

    // printf("[DEBUG] Buscando '%s' dentro del inodo %d. Resta: '%s'\n", inicial, *p_inodo_dir, final);

    // 3. Leemos el inodo del directorio actual para buscar en él
    if (leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO)
        return FALLO;

    // Verificamos que tenga permiso de lectura
    if ((inodo_dir.permisos & 4) != 4)
    {
        return ERROR_PERMISO_LECTURA;
    }

    // Inicializamos buffer de lectura para mayor eficiencia
    // Buffer para leer varias entradas a la vez si quisiéramos optimizar,
    // pero aquí leeremos una a una usando el offset lógico.
    // Número de entradas actuales en el directorio:
    cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);
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
            // Verificamos permisos de escritura en el directorio padre
            if ((inodo_dir.permisos & 2) != 2)
            {
                return ERROR_PERMISO_ESCRITURA;
            }

            // Reservamos un nuevo inodo para el fichero/directorio nuevo
            // NOTA: Para distinguir si es fichero o directorio usamos el 'tipo' extraído
            // 'd' si había slash al final (extraer_camino lo detecta si el usuario pone /ruta/)
            // o basándonos en el argumento 'permisos' o 'tipo' pasado (que no tenemos explícito en args).
            // Usaremos el argumento 'permisos' del padre o una lógica estándar.
            // Generalmente: 'permisos' viene dado.
            // Asumiremos que el caller sabe qué permisos poner.
            unsigned char tipo_inodo = 'f';
            if (permisos == 6)
                tipo_inodo = 'f'; // rw- -> fichero
            else if (permisos == 7)
                tipo_inodo = 'd'; // rwx -> directorio (heuristic)
            // *Mejora*: Si tipo (extraído en extraer_camino) detectó que 'final' no estaba vacío, es dir,
            // pero aquí final está vacío. Nos fiamos de los permisos pasados.

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