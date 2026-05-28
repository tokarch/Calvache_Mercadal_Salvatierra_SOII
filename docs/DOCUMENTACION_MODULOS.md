# DOCUMENTACIÓN DE MÓDULOS — Sistema de Ficheros por Inodos (SOII)

> Proyecto educativo para Sistemas Operativos II. Implementación en C de un sistema de ficheros basado en inodos sobre un disco virtual (fichero plano).

---

## Índice

1. [Cabeceras (Headers)](#1-cabeceras-headers)
   1. [bloques.h](#11-bloquesh)
   2. [ficheros_basico.h](#12-ficheros_basicoh)
   3. [ficheros.h](#13-ficherosh)
   4. [directorios.h](#14-directoriosh)
   5. [simulacion.h](#15-simulacionh)
   6. [verificacion.h](#16-verificacionh)
   7. [semaforo_mutex_posix.h](#17-semaforo_mutex_posixh)
2. [Módulos Core](#2-módulos-core)
   8. [bloques.c](#21-bloquesc)
   9. [ficheros_basico.c](#22-ficheros_basicoc)
   10. [ficheros.c](#23-ficherosc)
   11. [directorios.c](#24-directoriosc)
   12. [semaforo_mutex_posix.c](#25-semaforo_mutex_posixc)
3. [Comandos de Usuario](#3-comandos-de-usuario)
   13. [mi_mkfs.c](#31-mi_mkfsc)
   14. [mi_ls.c](#32-mi_lsc)
   15. [mi_mkdir.c](#33-mi_mkdirc)
   16. [mi_touch.c](#34-mi_touchc)
   17. [mi_chmod.c](#35-mi_chmodc)
   18. [mi_link.c](#36-mi_linkc)
   19. [mi_rm.c](#37-mi_rmc)
   20. [mi_rmdir.c](#38-mi_rmdirc)
   21. [mi_escribir.c](#39-mi_escribirc)
   22. [mi_cat.c](#310-mi_catc)
   23. [mi_stat.c](#311-mi_statc)
4. [Utilidades](#4-utilidades)
   24. [leer_sf.c](#41-leer_sfc)
   25. [escribir.c](#42-escribirc)
   26. [leer.c](#43-leerc)
   27. [truncar.c](#44-truncarc)
   28. [permitir.c](#45-permitirc)
   29. [prueba_nivel7.c](#46-prueba_nivel7c)
5. [Concurrencia](#5-concurrencia)
   30. [simulacion.c](#51-simulacionc)
   31. [verificacion.c](#52-verificacionc)

---

# 1. Cabeceras (Headers)

## 1.1 `bloques.h`

### Introducción del módulo

Cabecera de nivel más bajo del sistema. Define las constantes fundamentales (`BLOCKSIZE`, `EXITO`, `FALLO`), macros de color ANSI para depuración visual, y los prototipos de las funciones de E/S a nivel de bloque (`bmount`, `bumount`, `bread`, `bwrite`). También declara los wrappers de semáforo `mi_waitSem`/`mi_signalSem` para la concurrencia (Nivel 11).

Incluye `<stdio.h>`, `<stdlib.h>`, `<unistd.h>`, `<sys/types.h>`, `<sys/stat.h>`, `<fcntl.h>`, `<errno.h>`, `<string.h>` y `semaforo_mutex_posix.h`. Esto significa que cualquier fichero que incluya `bloques.h` hereda toda la cadena de dependencias POSIX necesarias para E/S de ficheros.

### Explicación detallada

**Constantes clave:**
- `BLOCKSIZE 1024`: Tamaño fijo de cada bloque del disco virtual. Todas las operaciones de lectura/escritura se realizan en múltiplos de este valor.
- `EXITO 0` / `FALLO -1`: Códigos de retorno estandarizados en todo el proyecto.

**Macros de color:** Secuencias ANSI escape para colorear la salida de depuración (`RED`, `GREEN`, `BLUE`, etc.) y `RESET` para restaurar. Se usan extensivamente en los `fprintf` condicionales controlados por `DEBUGN3`–`DEBUGN8`.

**Prototipos:**
| Función | Firma | Propósito |
|---------|-------|-----------|
| `bmount` | `int bmount(const char *camino)` | Abre/monta el fichero disco |
| `bumount` | `int bumount()` | Cierra/desmonta el disco |
| `bwrite` | `int bwrite(unsigned int nbloque, const void *buf)` | Escribe 1 bloque (1024 bytes) |
| `bread` | `int bread(unsigned int nbloque, void *buf)` | Lee 1 bloque (1024 bytes) |
| `mi_waitSem` | `void mi_waitSem()` | Entra en sección crítica |
| `mi_signalSem` | `void mi_signalSem()` | Sale de sección crítica |

### Intuición

Separar `BLOCKSIZE` como macro permite cambiar el tamaño de bloque recompilando sin tocar la lógica. Los colores de depuración no son decorativos: en un sistema con cientos de trazas `fprintf`, distinguir visualmente lecturas (negro), escrituras (naranja), y liberaciones (rojo) acelera enormemente el debugging.

### Riesgos y edge cases

- **Dependencia transitiva:** Incluir `bloques.h` arrastra todas las cabeceras POSIX. No hay problema en este proyecto monolítico, pero en un proyecto mayor sería preferible separar.
- **`mi_waitSem`/`mi_signalSem`** se declaran aquí pero se implementan en `bloques.c`. Si alguien las llamara sin haber hecho `bmount` primero, el semáforo sería `NULL` y el comportamiento es indefinido.

### Resumen técnico final

`bloques.h` es la piedra angular del sistema: define el tamaño de bloque, los códigos de error, y la interfaz de E/S bruta. Todo módulo del proyecto depende directa o indirectamente de esta cabecera.

---

## 1.2 `ficheros_basico.h`

### Introducción del módulo

Define las estructuras de datos fundamentales del sistema de ficheros: `struct superbloque` y `struct inodo`, junto con las constantes de direccionamiento multinivel y los prototipos de funciones de gestión de bajo nivel (bitmap, inodos, traducción de bloques).

### Explicación detallada

**Constantes de posicionamiento:**
- `posSB 0`, `tamSB 1`: El superbloque siempre ocupa el bloque 0 del disco.

**Constantes de inodo:**
- `INODOSIZE 128`: Cada inodo ocupa exactamente 128 bytes. Caben 8 inodos por bloque (1024/128).

**Constantes de direccionamiento multinivel:**
- `NPUNTEROS 256`: Punteros que caben en un bloque índice (1024 / sizeof(unsigned int) = 1024/4).
- `DIRECTOS 12`: Punteros directos en el inodo → cubren bloques lógicos 0–11 (hasta 12 KB).
- `INDIRECTOS0 268`: 12 + 256 → hasta ~268 KB.
- `INDIRECTOS1 65.804`: 268 + 256² → hasta ~64 MB.
- `INDIRECTOS2 16.843.020`: 65.804 + 256³ → hasta ~16 GB.

**`struct superbloque`** (ocupa 1 bloque = 1024 bytes):
| Campo | Tipo | Significado |
|-------|------|-------------|
| `posPrimerBloqueMB` | `unsigned int` | Inicio del mapa de bits |
| `posUltimoBloqueMB` | `unsigned int` | Fin del mapa de bits |
| `posPrimerBloqueAI` | `unsigned int` | Inicio del array de inodos |
| `posUltimoBloqueAI` | `unsigned int` | Fin del array de inodos |
| `posPrimerBloqueDatos` | `unsigned int` | Inicio de la zona de datos |
| `posUltimoBloqueDatos` | `unsigned int` | Fin de la zona de datos |
| `posInodoRaiz` | `unsigned int` | Inodo del directorio raíz (relativo al AI) |
| `posPrimerInodoLibre` | `unsigned int` | Cabeza de la lista de inodos libres |
| `cantBloquesLibres` | `unsigned int` | Bloques libres restantes |
| `cantInodosLibres` | `unsigned int` | Inodos libres restantes |
| `totBloques` | `unsigned int` | Total de bloques del disco |
| `totInodos` | `unsigned int` | Total de inodos |
| `padding` | `char[...]` | Relleno hasta 1024 bytes |

**`struct inodo`** (128 bytes exactos):
| Campo | Tipo | Significado |
|-------|------|-------------|
| `tipo` | `unsigned char` | `'l'` libre, `'d'` directorio, `'f'` fichero |
| `permisos` | `unsigned char` | Bits rwx (0–7) |
| `reservado_alineacion[6]` | `unsigned char` | Alineación a 64 bits |
| `atime` | `time_t` | Último acceso a datos |
| `mtime` | `time_t` | Última modificación de datos |
| `ctime` | `time_t` | Última modificación del inodo |
| `btime` | `time_t` | Creación del inodo (birth) |
| `nlinks` | `unsigned int` | Número de enlaces duros |
| `tamEnBytesLog` | `unsigned int` | Tamaño lógico (EOF) |
| `numBloquesOcupados` | `unsigned int` | Bloques físicos ocupados |
| `punterosDirectos[12]` | `unsigned int` | Punteros a bloques directos |
| `punterosIndirectos[3]` | `unsigned int` | Indirecto simple, doble, triple |
| `padding` | `char[...]` | Relleno hasta 128 bytes |

**Prototipos por nivel:**
- **Nivel 2:** `tamMB`, `tamAI`, `initSB`, `initMB`, `initAI`
- **Nivel 3:** `escribir_bit`, `leer_bit`, `reservar_bloque`, `liberar_bloque`, `escribir_inodo`, `leer_inodo`, `reservar_inodo`
- **Nivel 4:** `traducir_bloque_inodo`
- **Nivel 6:** `liberar_inodo`, `liberar_bloques_inodo`

### Intuición

El diseño del superbloque con posiciones absolutas permite que cualquier función pueda localizar cualquier zona del disco leyendo un solo bloque (el 0). La lista enlazada de inodos libres usa `punterosDirectos[0]` como "siguiente libre", reutilizando espacio que de otro modo estaría desperdiciado en inodos no asignados.

Los umbrales `INDIRECTOS0/1/2` son acumulativos, no independientes. Esto simplifica `obtener_nRangoBL`: basta con comparaciones `<` encadenadas.

### Riesgos y edge cases

- **`time_t` en 64 bits:** Ocupa 8 bytes, no 4. El `padding` del inodo se calcula dinámicamente con `sizeof(time_t)` para garantizar 128 bytes exactos. Si `time_t` cambiara de tamaño, el padding se ajustaría automáticamente.
- **`UINT_MAX` como NULL:** El último inodo libre usa `UINT_MAX` en `punterosDirectos[0]` como centinela de fin de lista. Si se usara `0`, colisionaría con un inodo válido.
- **Desbordamiento en `INDIRECTOS2`:** 16.843.020 bloques × 1024 bytes ≈ 16 GB. El fichero virtual máximo está limitado por este valor.

### Resumen técnico final

`ficheros_basico.h` es el diccionario de datos del sistema. Define cómo se organizan los metadatos en disco (superbloque, inodos) y cómo se direccionan los bloques de datos (multinivel). Sin estas estructuras, no existe sistema de ficheros.

---

## 1.3 `ficheros.h`

### Introducción del módulo

Puente entre la capa de inodos (`ficheros_basico`) y la capa de directorios (`directorios`). Define `struct STAT` (equivalente simplificado de `stat(2)` de POSIX) y los prototipos de operaciones a nivel de fichero: lectura, escritura, estadísticas, cambio de permisos y truncado.

### Explicación detallada

**`struct STAT`:** Versión simplificada del inodo para consumo externo. Contiene solo los campos que un usuario necesita consultar: tipo, permisos, 3 timestamps, nlinks, tamaño lógico y bloques ocupados. No expone los punteros internos.

**Prototipos:**
| Función | Nivel | Propósito |
|---------|-------|-----------|
| `mi_write_f` | 5 | Escribe bytes en un inodo desde offset |
| `mi_read_f` | 5 | Lee bytes de un inodo desde offset |
| `mi_stat_f` | 5 | Rellena un `struct STAT` con metadatos |
| `mi_chmod_f` | 5 | Cambia permisos de un inodo |
| `mi_truncar_f` | 6 | Trunca un fichero a N bytes |

### Intuición

La separación `mi_write_f`/`mi_read_f` (trabajan con número de inodo) frente a `mi_write`/`mi_read` en `directorios.h` (trabajan con ruta) sigue el principio de separación de responsabilidades: `ficheros.c` no sabe de caminos ni directorios, solo de bloques lógicos y físicos.

### Riesgos y edge cases

- `mi_write_f` y `mi_read_f` gestionan su propia sección crítica con `mi_waitSem`/`mi_signalSem`. Si se llamaran desde código que ya tiene el semáforo, habría deadlock. El contador `inside_sc` en `bloques.c` mitiga esto con reentrada.

### Resumen técnico final

`ficheros.h` expone la API de operaciones sobre contenido de ficheros. Es la interfaz que usan tanto los comandos de usuario como las funciones de directorio para leer/escribir datos reales.

---

## 1.4 `directorios.h`

### Introducción del módulo

Define las estructuras y constantes para la gestión de directorios: `struct entrada` (nombre + inodo), límites de profundidad y nombre, códigos de error específicos, y los prototipos de funciones de alto nivel (crear, listar, enlazar, desvincular).

### Explicación detallada

**Constantes:**
- `TAMNOMBRE 60`: Longitud máxima del nombre de fichero/directorio.
- `PROFUNDIDAD 32`: Profundidad máxima del árbol de directorios.
- `TAMFILA 300`: Ancho máximo de una línea en la salida de `mi_dir`.
- `TAMBUFFER (TAMFILA * 1000)`: Buffer para listar (~1000 entradas).

**`struct entrada`** (64 bytes):
```c
struct entrada {
    char nombre[TAMNOMBRE];  // 60 bytes
    unsigned int ninodo;     // 4 bytes
};
```
Caben 16 entradas por bloque (1024/64).

**Códigos de error:**
| Constante | Valor | Significado |
|-----------|-------|-------------|
| `ERROR_CAMINO_INCORRECTO` | -1 | Ruta no empieza por `/` |
| `ERROR_PERMISO_LECTURA` | -2 | Sin permiso de lectura |
| `ERROR_NO_EXISTE_ENTRADA` | -3 | Entrada no encontrada |
| `ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO` | -4 | Directorio padre inexistente |
| `ERROR_PERMISO_ESCRITURA` | -5 | Sin permiso de escritura |
| `ERROR_ENTRADA_YA_EXISTENTE` | -6 | Entrada duplicada / directorio no vacío |
| `ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_FICHERO` | -7 | Se intentó crear dentro de un fichero |

**Prototipos por nivel:**
- **Nivel 7:** `extraer_camino`, `buscar_entrada`, `mostrar_error_buscar_entrada`
- **Nivel 8:** `mi_creat`, `mi_dir`, `mi_chmod`, `mi_stat`
- **Nivel 9:** `mi_write`, `mi_read`
- **Nivel 10:** `mi_link`, `mi_unlink`

### Intuición

Los errores negativos se distinguen claramente de `EXITO` (0) y `FALLO` (-1). El código `-1` se reutiliza como `ERROR_CAMINO_INCORRECTO`, lo cual es coherente ya que `FALLO` genérico también es -1.

La convención de que los directorios terminan en `/` y los ficheros no es el mecanismo principal para distinguir tipos de ruta sin necesidad de consultar el inodo.

### Riesgos y edge cases

- `TAMBUFFER` de ~300 KB es grande pero fijo. Si un directorio tiene más de 1000 entradas, se trunca silenciosamente.
- `TAMNOMBRE 60` con `strncpy` protege contra overflow, pero nombres de exactamente 60 caracteres no tendrán null terminator si no se añade manualmente.

### Resumen técnico final

`directorios.h` es la interfaz de alto nivel del sistema. Expone las operaciones que un usuario final realizaría: crear ficheros, listar directorios, cambiar permisos por ruta, leer/escribir por nombre, y gestionar enlaces duros.

---

## 1.5 `simulacion.h`

### Introducción del módulo

Define las constantes y la estructura `REGISTRO` para la simulación de concurrencia (Nivel 12). Cada registro representa una escritura atómica de un proceso hijo al fichero `prueba.dat`.

### Explicación detallada

**Constantes:**
- `NUMPROCESOS 100`: Número de procesos hijos que se fork-ean.
- `NUMESCRITURAS 50`: Escrituras por proceso.
- `REGMAX 500000`: Rango máximo de posiciones de registro.

**`struct REGISTRO`** (32 bytes en sistemas de 64 bits):
```c
struct REGISTRO {
    struct timeval fecha;  // 16 bytes (tv_sec + tv_usec)
    pid_t pid;             // 4 bytes
    int nEscritura;        // 4 bytes (1–50)
    int nRegistro;         // 4 bytes (0–REGMAX-1)
};
```

### Intuición

`struct timeval` proporciona resolución de microsegundos, esencial para ordenar escrituras de procesos concurrentes que podrían ocurrir en el mismo segundo. El campo `nRegistro` permite escrituras en posiciones aleatorias, probando que el semáforo protege correctamente contra escrituras solapadas.

### Riesgos y edge cases

- `sizeof(struct REGISTRO)` puede variar entre arquitecturas (padding). La verificación debe leer por offset calculado como `nRegistro * sizeof(struct REGISTRO)`, no asumir un tamaño fijo.
- `rand() % REGMAX` no es uniformemente perfecto pero suficiente para simulación educativa.

### Resumen técnico final

`simulacion.h` define el formato de datos para el test de concurrencia. Cada registro es una prueba de que las escrituras concurrentes no se corrompen.

---

## 1.6 `verificacion.h`

### Introducción del módulo

Define `struct INFORMACION` para acumular métricas de verificación de cada proceso tras la simulación de concurrencia (Nivel 13).

### Explicación detallada

**`struct INFORMACION`:**
```c
struct INFORMACION {
    int pid;
    unsigned int nEscrituras;
    struct REGISTRO PrimeraEscritura;
    struct REGISTRO UltimaEscritura;
    struct REGISTRO MenorPosicion;
    struct REGISTRO MayorPosicion;
};
```

Cada campo `REGISTRO` almacena no solo el valor extremo sino el registro completo (con fecha y PID), lo que permite generar un informe detallado con timestamps.

### Intuición

Separar "primera/última escritura" (orden temporal por `nEscritura`) de "menor/mayor posición" (orden espacial por `nRegistro`) permite detectar dos tipos de problemas: escrituras fuera de orden y accesos a posiciones inesperadas.

### Riesgos y edge cases

- Si un proceso no escribe ningún registro válido, los campos `PrimeraEscritura` etc. quedan sin inicializar. El código de verificación usa un flag `primer_registro_valido` para evitar esto.

### Resumen técnico final

`verificacion.h` es mínima pero esencial: define la estructura de acumulación de métricas que `verificacion.c` usa para generar `informe.txt`.

---

## 1.7 `semaforo_mutex_posix.h`

### Introducción del módulo

Cabecera mínima que declara los prototipos de las 4 operaciones sobre semáforos POSIX con nombre: creación, eliminación, signal y wait.

### Explicación detallada

**Prototipos:**
| Función | Retorno | Propósito |
|---------|---------|-----------|
| `initSem()` | `sem_t *` | Crea/abre semáforo named con valor inicial 1 |
| `deleteSem(sem_t *)` | `void` | Cierra y desvincula el semáforo |
| `signalSem(sem_t *)` | `void` | `sem_post` — libera el mutex |
| `waitSem(sem_t *)` | `void` | `sem_wait` — adquiere el mutex |

### Intuición

Usar semáforos con nombre (`sem_open`) en lugar de semáforos anónimos (`sem_init`) es crucial porque los procesos hijos creados con `fork()` necesitan acceder al mismo semáforo. Los semáforos anónimos en memoria no se comparten entre procesos sin memoria compartida explícita.

### Riesgos y edge cases

- El nombre `/sem_mi_sf` es hardcodeado. Si dos instancias del FS se ejecutan simultáneamente en el mismo sistema, compartirán el mismo semáforo, lo cual es correcto (protege el mismo disco).
- `sem_unlink` puede fallar con `ENOENT` si otro proceso ya lo desvinculó. Esto es esperado, no error.

### Resumen técnico final

`semaforo_mutex_posix.h` define la interfaz de sincronización. Sin ella, no hay protección contra accesos concurrentes al disco virtual.

---

# 2. Módulos Core

## 2.1 `bloques.c`

### Introducción del módulo

Implementa la capa de E/S más baja del sistema: montar/desmontar el disco virtual y leer/escribir bloques de 1024 bytes. Es el equivalente al driver de dispositivo en un sistema de ficheros real.

### Explicación detallada

**Variables estáticas:**
```c
static int descriptor = 0;        // FD del fichero disco
static sem_t *mutex;              // Semáforo global
static unsigned int inside_sc = 0; // Contador de reentrada
```
`descriptor` es `static` para encapsulación: solo las funciones de `bloques.c` pueden acceder al FD directamente.

**`bmount(const char *camino)`:**
1. Si `descriptor > 0` (heredado de `fork`), lo cierra para evitar compartir el puntero `lseek` entre padre e hijos.
2. `umask(0000)` para que los permisos solicitados (0666) no sean modificados por la máscara del sistema.
3. `open(camino, O_RDWR | O_CREAT, 0666)` abre o crea el fichero disco.
4. Si `mutex` es NULL, inicializa el semáforo con `initSem()`.
5. Retorna el descriptor o `FALLO`.

**`bumount()`:**
1. `deleteSem(mutex)` cierra y desvincula el semáforo.
2. `close(descriptor)` libera el FD.
3. Retorna `EXITO` o `FALLO`.

**`bwrite(unsigned int nbloque, const void *buf)`:**
1. Calcula `desplazamiento = (off_t)nbloque * BLOCKSIZE`. El cast a `off_t` evita desbordamiento en discos grandes.
2. `lseek(descriptor, desplazamiento, SEEK_SET)` posiciona el puntero de fichero.
3. `write(descriptor, buf, BLOCKSIZE)` escribe exactamente 1024 bytes.
4. Retorna bytes escritos o `FALLO`.

**`bread(unsigned int nbloque, void *buf)`:**
Idéntico a `bwrite` pero con `read()` en lugar de `write()`.

**`mi_waitSem()` / `mi_signalSem()`:**
```c
void mi_waitSem() {
    if (inside_sc == 0) waitSem(mutex);
    inside_sc++;
}
void mi_signalSem() {
    inside_sc--;
    if (inside_sc == 0) signalSem(mutex);
}
```
El contador `inside_sc` permite reentrada: si una función que ya tiene el semáforo llama a otra que también lo necesita, no se produce deadlock. El semáforo real solo se libera cuando `inside_sc` vuelve a 0.

### Intuición

El diseño de un único descriptor global simplifica enormemente la API: no hay que pasar el FD a cada función. La contrapartida es que solo se puede tener un disco montado a la vez, lo cual es aceptable para un proyecto educativo.

La reentrada con `inside_sc` es un patrón clásico de semáforo recursivo. POSIX no soporta semáforos recursivos nativamente, así que se simula con un contador.

### Riesgos y edge cases

- **`fork()` y descriptor compartido:** Tras un `fork()`, padre e hijos comparten el mismo FD y, por tanto, el mismo puntero de fichero. Si el padre hace `lseek` mientras el hijo escribe, los datos se corrompen. Por eso `bmount` cierra y reabre el descriptor en los hijos.
- **`inside_sc` no es atómico:** En un sistema real con threads, se necesitaría `atomic_uint`. Aquí solo hay procesos con `fork`, y el semáforo POSIX protege el acceso.
- **`bumount` no resetea `mutex` a NULL:** Si se llama a `bmount` después de `bumount`, `mutex` sigue apuntando a memoria liberada. En la práctica esto no ocurre porque los comandos montan y desmontan una sola vez.

### Resumen técnico final

`bloques.c` es la abstracción del dispositivo de bloque. Traduce números de bloque lógico en offsets de fichero y usa `read`/`write` POSIX. El semáforo con reentrada protege todas las operaciones críticas.

---

## 2.2 `ficheros_basico.c`

### Introducción del módulo

El módulo más grande y complejo del proyecto (1279 líneas). Implementa toda la gestión de metadatos: inicialización del superbloque, mapa de bits y array de inodos; reserva y liberación de bloques e inodos; y la traducción de bloques lógicos a físicos mediante direccionamiento multinivel.

### Explicación detallada

#### Nivel 2: Inicialización

**`tamMB(nbloques)` y `tamAI(ninodos)`:** Calculan cuántos bloques necesita el mapa de bits y el array de inodos respectivamente. La fórmula es `(total_bytes / BLOCKSIZE) + (resto ? 1 : 0)`. Para el MB: `nbloques / 8` bytes (1 bit por bloque). Para el AI: `ninodos * 128` bytes.

**`initSB(nbloques, ninodos)`:** Construye el superbloque en memoria y lo escribe en el bloque 0. Las zonas se ubican secuencialmente: SB (bloque 0) → MB → AI → Datos. Inicialmente todos los bloques e inodos se marcan como libres.

**`initMB()`:** Marca los bloques de metadatos (SB + MB + AI) como ocupados en el bitmap. Itera bloque a bloque del MB, calculando bytes completos y bits restantes. Resta los metadatos de `cantBloquesLibres` en el SB.

**`initAI()`:** Inicializa todos los inodos como `'l'` (libre) y los enlaza en una lista usando `punterosDirectos[0]` como puntero al siguiente libre. El último inodo apunta a `UINT_MAX` (fin de lista).

#### Nivel 3: Bitmap e Inodos

**`escribir_bit(nbloque, bit)`:** Localiza el byte y bit exacto en el MB correspondiente a `nbloque`. Usa una máscara `128 >> posbit` para poner a 1 (OR) o a 0 (AND con NOT). Lee el bloque del MB, modifica el byte, y lo reescribe.

**`leer_bit(nbloque)`:** Similar pero solo lee. Aplica AND con la máscara y desplaza el resultado para devolver 0 o 1.

**`reservar_bloque()`:** Busca el primer bit a 0 en el MB (primer bloque libre). Optimización: compara cada bloque del MB con un buffer de 0xFF usando `memcmp`; si son iguales, todo el bloque está ocupado y se salta. Una vez encontrado el byte y bit libres, calcula el número de bloque lógico, lo marca como ocupado con `escribir_bit`, decrementa `cantBloquesLibres`, y limpia el bloque de datos con ceros. Toda la operación está protegida por `mi_waitSem`/`mi_signalSem`.

**`liberar_bloque(nbloque)`:** Pone el bit a 0 con `escribir_bit` e incrementa `cantBloquesLibres`. También protegido por semáforo.

**`escribir_inodo(ninodo, inodo)` y `leer_inodo(ninodo, inodo)`:** Calculan en qué bloque del AI está el inodo: `nbloqueAI = (ninodo * 128) / 1024`. Leen/escriben el bloque completo del AI (8 inodos) y acceden al índice `ninodo % 8`.

**`reservar_inodo(tipo, permisos)`:** Usa la lista enlazada de inodos libres (LIFO). Lee `posPrimerInodoLibre` del SB, lee ese inodo para obtener el siguiente libre (`punterosDirectos[0]`), actualiza el SB para que apunte al siguiente, inicializa el inodo con tipo, permisos, `nlinks=1`, timestamps actuales, y punteros a 0. Protegido por semáforo.

#### Nivel 4: Traducción Multinivel

**`obtener_nRangoBL(inodo, nblogico, ptr)`:** Determina en qué rango cae un bloque lógico (0=directo, 1=indirecto simple, 2=doble, 3=triple) y devuelve el puntero inicial correspondiente.

**`obtener_indice(nblogico, nivel_punteros)`:** Calcula el índice dentro de un bloque de punteros. Para indirecto doble y triple, usa división y módulo en base 256 para descomponer la posición.

**`traducir_bloque_inodo(ninodo, nblogico, reservar)`:** Función central del sistema. Navega el árbol de punteros desde el nivel más alto hasta el bloque de datos:
1. Lee el inodo y determina el rango.
2. Bucle `while (nivel_punteros > 0)`: en cada iteración, si el puntero actual es 0 y `reservar=1`, crea un nuevo bloque índice. Si `reservar=0`, retorna `FALLO`.
3. Calcula el índice, avanza al hijo (`ptr = buffer[indice]`), decrementa nivel.
4. Al salir del bucle (nivel 0 = datos), si el puntero es 0 y `reservar=1`, reserva un bloque de datos.
5. Si se modificó el inodo, lo escribe con `escribir_inodo`.
6. Retorna el número de bloque físico.

#### Nivel 6: Liberación

**`liberar_directos(nBL, ultimoBL, inodo, eof)`:** Itera los 12 punteros directos. Si `nBL` está dentro del rango `[primerBL, ultimoBL]` y el puntero no es 0, libera el bloque y lo pone a 0.

**`liberar_indirectos_recursivo(nBL, primerBL, ultimoBL, puntero_bloque, nivel_punteros, eof)`:** Función recursiva que libera bloques de datos e índices. Optimización clave: si `*puntero_bloque == 0` o todo el rango del subárbol está antes de `primerBL`, salta sin leer disco (`*nBL += saltos`). Tras liberar hijos, usa `memcmp` contra buffer de ceros: si el bloque índice quedó vacío, lo libera también (propagación hacia arriba).

**`liberar_bloques_inodo(primerBL, inodo)`:** Orquestador. Primero libera directos iterativamente, luego llama a `liberar_indirectos_recursivo` para cada nivel indirecto (1, 2, 3). El flag `eof` evita procesar niveles superiores si ya se pasó del último bloque.

**`liberar_inodo(ninodo)`:** Libera un inodo completamente:
1. Lee el inodo.
2. Llama a `liberar_bloques_inodo(0, &inodo)` para liberar todos los bloques.
3. Resetea el inodo: `tipo='l'`, `tamEnBytesLog=0`, `numBloquesOcupados=0`.
4. Inserta el inodo en la cabeza de la lista de libres (LIFO): `inodo.punterosDirectos[0] = SB.posPrimerInodoLibre; SB.posPrimerInodoLibre = ninodo`.
5. Escribe el inodo y el SB. Protegido por semáforo.

### Intuición

La traducción multinivel es el corazón del sistema. El diseño iterativo (bucle `while`) en lugar de recursivo para la navegación hacia abajo evita el overhead de llamadas recursivas y simplifica la gestión de memoria. La liberación recursiva hacia arriba es natural porque necesita verificar si un bloque índice quedó vacío después de liberar sus hijos.

La optimización de salto en `liberar_indirectos_recursivo` es crucial: sin ella, truncar un fichero de 1 GB requeriría leer miles de bloques índice vacíos. Con el salto, se saltan rangos completos sin acceso a disco.

### Riesgos y edge cases

- **`reservar_bloque` lee el SB dentro de la sección crítica:** Si otra operación también lee el SB, no hay problema porque el semáforo serializa. Pero el SB se lee múltiples veces dentro de una misma operación (en `escribir_bit`, en `leer_inodo`), lo cual es ineficiente.
- **`liberar_indirectos_recursivo` con `nivel_punteros > 3`:** No está protegido explícitamente. Si se llamara con un nivel inválido, el bucle de `saltos` podría desbordar.
- **`escribir_bit` lee y escribe el bloque del MB para cada bit:** En un sistema real, se mantendría el MB en memoria y se volcaría periódicamente. Aquí cada operación de bit implica 2 accesos a disco (lectura + escritura del bloque MB).
- **`liberar_bloque` no verifica que el bloque esté realmente ocupado:** Si se libera un bloque ya libre, el bitmap se corrompe (bit ya era 0).

### Resumen técnico final

`ficheros_basico.c` es el motor del sistema de ficheros. Gestiona la asignación de recursos (bloques e inodos), el bitmap de ocupación, y la traducción de direcciones lógicas a físicas mediante un árbol de punteros multinivel de hasta 4 niveles. La liberación recursiva con optimización de salto minimiza los accesos a disco innecesarios.

---

## 2.3 `ficheros.c`

### Introducción del módulo

Implementa las operaciones de lectura y escritura a nivel de fichero (bytes lógicos), gestionando la fragmentación de datos a través de múltiples bloques físicos. Es la capa que traduce "escribir N bytes desde el offset X" en operaciones de bloque.

### Explicación detallada

**`mi_write_f(ninodo, buf_original, offset, nbytes)`:**
1. Entra en sección crítica (`mi_waitSem`).
2. Lee el inodo y verifica permiso de escritura (bit 2).
3. Calcula `primerBL = offset / BLOCKSIZE` y `ultimoBL = (offset + nbytes - 1) / BLOCKSIZE`.
4. Calcula desplazamientos dentro del primer y último bloque: `desp1 = offset % BLOCKSIZE`, `desp2 = (offset + nbytes - 1) % BLOCKSIZE`.
5. **Caso 1 (un solo bloque):** Si `primerBL == ultimoBL`, lee el bloque físico con `traducir_bloque_inodo(..., reservar=1)`, copia los bytes con `memcpy` en la posición correcta, y escribe el bloque.
6. **Caso 2 (múltiples bloques):**
   - **Primer bloque:** Lee, copia desde `desp1` hasta el final del bloque, escribe.
   - **Bloques intermedios:** Para cada bloque completo, obtiene el bloque físico, lee, copia el bloque entero, escribe.
   - **Último bloque:** Lee, copia desde el inicio hasta `desp2`, escribe.
7. Si la escritura extiende el fichero (`offset + nbytes > tamEnBytesLog`), actualiza `tamEnBytesLog` y `ctime`.
8. Actualiza `mtime` y escribe el inodo.
9. Sale de sección crítica (`mi_signalSem`).

**`mi_read_f(ninodo, buf_original, offset, nbytes)`:**
Similar a `mi_write_f` pero en sentido inverso:
1. Verifica permiso de lectura (bit 4).
2. Si `offset >= tamEnBytesLog`, retorna 0 (EOF).
3. Si `offset + nbytes > tamEnBytesLog`, recorta `nbytes` al EOF.
4. Para bloques que no existen físicamente (`traducir_bloque_inodo` retorna -1), rellena con ceros (`memset`) en lugar de fallar. Esto permite leer "huecos" en ficheros sparse.
5. Actualiza `atime` al final.

**`mi_stat_f(ninodo, p_stat)`:** Lee el inodo y copia los campos relevantes en `struct STAT`. Simple.

**`mi_chmod_f(ninodo, permisos)`:** Lee el inodo, cambia `permisos`, actualiza `ctime`, escribe el inodo.

**`mi_truncar_f(ninodo, nbytes)`:**
1. Verifica permiso de escritura.
2. Si `nbytes > tamEnBytesLog`, error (no se puede truncar más allá del EOF).
3. Calcula `primerBL` (primer bloque lógico a liberar).
4. Llama a `liberar_bloques_inodo(primerBL, &inodo)`.
5. Actualiza `tamEnBytesLog = nbytes`, `numBloquesOcupados -= liberados`, `mtime` y `ctime`.
6. Escribe el inodo.

### Intuición

La fragmentación en 3 fases (primer bloque parcial, bloques intermedios completos, último bloque parcial) es el patrón estándar de E/S fragmentada. Permite escribir eficientemente sin leer bloques que no se van a modificar (en el caso de bloques intermedios completos, se lee y reescribe el bloque entero porque `bwrite` opera a nivel de bloque).

El tratamiento de bloques inexistentes como ceros en `mi_read_f` soporta ficheros sparse: si se escribe en el offset 1.000.000 sin llenar los anteriores, leer los huecos devuelve ceros sin error.

### Riesgos y edge cases

- **`mi_write_f` lee bloques intermedios completos antes de escribirlos:** Si el bloque no existía, `traducir_bloque_inodo` con `reservar=0` retorna -1, y el código salta la lectura.
- **Granularidad fina:** Al haber sacado los `bread` y `bwrite` de datos fuera de la sección crítica, se ha eliminado el riesgo de cuello de botella. El sistema es ahora altamente concurrente.

### Resumen técnico final

`ficheros.c` traduce operaciones de bytes en operaciones de bloques. Gestiona fragmentación, permisos, timestamps, y truncado. Es la capa que hace que el sistema de ficheros sea usable a nivel de contenido.

---

## 2.4 `directorios.c`

### Introducción del módulo

Implementa la capa de abstracción de directorios: resolución de rutas, creación de entradas, listado, lectura/escritura por nombre, y gestión de enlaces duros. Es la capa más cercana al usuario.

### Explicación detallada

**Caché de última entrada:**
```c
struct UltimaEntrada {
    char camino[TAMNOMBRE * PROFUNDIDAD];
    int p_inodo;
};
static struct UltimaEntrada UltimaEntradaLectura;
static struct UltimaEntrada UltimaEntradaEscritura;
```
Dos cachés separadas (lectura y escritura) evitan que una operación de lectura invalide el resultado de una escritura previa sobre el mismo fichero.

**`extraer_camino(camino, inicial, final, tipo)`:** Separa el primer componente de una ruta. Ejemplo: `/home/user/docs` → `inicial="home"`, `final="/user/docs"`, `tipo='d'`. Si es el último componente (`/fichero`), `final=""` y `tipo='f'`. Usa `strchr` para encontrar el siguiente `/`.

**`buscar_entrada(camino_parcial, p_inodo_dir, p_inodo, p_entrada, reservar, permisos)`:** Función recursiva central:
1. Llama a `extraer_camino` para obtener `inicial` y `final`.
2. Lee el inodo del directorio actual (`*p_inodo_dir`). Verifica permiso de lectura.
3. Si el inodo no es de tipo directorio, retorna `ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_FICHERO`.
4. Busca `inicial` en las entradas del directorio leyendo bloques completos con `mi_read_f`.
5. **Si encontrado:**
   - Si `final == ""` y `reservar == 1`: error `ERROR_ENTRADA_YA_EXISTENTE`.
   - Si `final == ""` y `reservar == 0`: éxito.
   - Si `final != ""`: recursión con `*p_inodo_dir = *p_inodo`.
6. **Si no encontrado:**
   - Si `final != ""`: error `ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO`.
   - Si `final == ""` y `reservar == 1`: crea nuevo inodo, nueva entrada, la escribe al final del directorio con `mi_write_f`.
   - Si `final == ""` y `reservar == 0`: error `ERROR_NO_EXISTE_ENTRADA`.

**`mostrar_error_buscar_entrada(error)`:** Switch que imprime el mensaje legible para cada código de error.

**`mi_creat(camino, permisos)`:** Wrapper que llama a `buscar_entrada` con `reservar=1`.

**`mi_dir(camino, buffer, tipo)`:**
- Si es fichero: formatea una línea con tipo, permisos, mtime, tamaño y nombre.
- Si es directorio: itera todas las entradas, lee cada inodo, formatea líneas con colores (azul para directorios, verde para ficheros), y las concatena en `buffer`.

**`mi_chmod(camino, permisos)` y `mi_stat(camino, p_stat)`:** Buscan la entrada y delegan en `mi_chmod_f`/`mi_stat_f`.

**`mi_write(camino, buf, offset, nbytes)` y `mi_read(camino, buf, offset, nbytes)`:** Usan la caché de última entrada para evitar resolver la ruta en cada llamada. Si el camino coincide con el cacheado, usan el inodo directamente. Si no, llaman a `buscar_entrada` con `reservar=0`.

**`mi_link(camino1, camino2)`:**
1. Verifica que `camino1` existe y es fichero (no directorio).
2. Usa el "truco" de llamar a `buscar_entrada(camino2, reservar=1)`. Esto crea una entrada y un inodo nuevo automáticamente.
3. Lee la entrada recién creada, modifica su puntero para que apunte al inodo de `camino1`, y la reescribe.
4. Libera el inodo temporal creado en el paso 2 con `liberar_inodo`.
5. Incrementa `nlinks` del inodo origen. Todo protegido por semáforos.

**`mi_unlink(camino)`:**
1. Busca la entrada a borrar.
2. Si es directorio y no está vacío (`tamEnBytesLog > 0`), error.
3. Si no es la última entrada del directorio padre, copia la última entrada en la posición de la borrada (compactación O(1)).
4. Trunca el directorio padre para eliminar la última posición.
5. Decrementa `nlinks` del inodo borrado. Si llega a 0, llama a `liberar_inodo`.

### Intuición

La compactación de `mi_unlink` (copiar la última entrada sobre la borrada) es O(1) en lugar de O(n) porque evita desplazar todas las entradas siguientes. La contrapartida es que el orden de las entradas cambia, pero esto es aceptable en un sistema de ficheros (el orden no es semántico).

La caché de última entrada es una optimización práctica: en patrones de uso real (escribir varias veces en el mismo fichero), evita resolver la ruta repetidamente.

### Riesgos y edge cases

- **`buscar_entrada` recursiva:** Con `PROFUNDIDAD 32`, la recursión máxima es 32 niveles. No hay riesgo de stack overflow, pero cada nivel lee el inodo del directorio, lo cual es costoso.
- **`mi_unlink` y hard links:** Si un fichero tiene múltiples hard links, `mi_unlink` solo decrementa `nlinks`. Los datos solo se liberan cuando `nlinks == 0`. Esto es correcto pero significa que el espacio no se recupera hasta que se borren todos los enlaces.
- **`mi_dir` con buffer fijo:** Si un directorio tiene muchas entradas, `buffer` puede llenarse y las entradas restantes se pierden silenciosamente.
- **`mi_link` no verifica permisos del directorio destino:** Solo verifica permisos de escritura del directorio padre después de `buscar_entrada`, pero `buscar_entrada` con `reservar=0` no verifica escritura.

### Resumen técnico final

`directorios.c` es la capa de abstracción que convierte rutas en operaciones sobre inodos. La resolución recursiva de caminos, la caché de última entrada, y la compactación O(1) en unlink son las decisiones de diseño más relevantes.

---

## 2.5 `semaforo_mutex_posix.c`

### Introducción del módulo

Implementación de las 4 operaciones de semáforo POSIX con nombre. Usa `sem_open` con el nombre `/sem_mi_sf` para crear un semáforo compartido entre procesos.

### Explicación detallada

**`initSem()`:** `sem_open("/sem_mi_sf", O_CREAT, 0644, 1)` crea o abre el semáforo con valor inicial 1 (mutex). Retorna `SEM_FAILED` en error.

**`deleteSem(sem)`:** `sem_close(sem)` cierra la referencia del proceso. `sem_unlink(SEM_NAME)` desvincula el nombre del sistema. Si `sem_unlink` falla con `ENOENT`, se ignora (otro proceso ya lo desvinculó).

**`signalSem(sem)`:** Wrapper de `sem_post`.

**`waitSem(sem)`:** Wrapper de `sem_wait`.

### Intuición

Los wrappers con `perror` proporcionan diagnóstico automático si algo falla. En un proyecto educativo, esto es más útil que retornar códigos de error que nadie verifica.

### Riesgos y edge cases

- **Nombre hardcodeado:** `/sem_mi_sf` es fijo. Si otro programa usa el mismo nombre, habrá conflicto.
- **`sem_unlink` prematuro:** Si un proceso llama a `deleteSem` mientras otros aún usan el semáforo, el nombre se desvincula pero el semáforo persiste hasta que el último proceso lo cierre. Esto es correcto por diseño POSIX.

### Resumen técnico final

Implementación mínima y correcta de semáforo mutex POSIX. Los wrappers añaden diagnóstico sin complejidad.

---

# 3. Comandos de Usuario

## 3.1 `mi_mkfs.c`

### Introducción del módulo

Formatea e inicializa el disco virtual. Equivale a `mkfs` en sistemas Unix. Crea el superbloque, bitmap, array de inodos, y reserva el inodo raíz.

### Explicación detallada

1. Valida argumentos: `mi_mkfs <disco> <nbloques>`.
2. Calcula `ninodos = nbloques / 4` (heurística del enunciado).
3. `bmount(nombre_dispositivo)` abre el fichero disco.
4. Escribe ceros en todos los bloques con un bucle `for` llamando a `bwrite`.
5. `initSB(nbloques, ninodos)` crea el superbloque.
6. `initMB()` marca los bloques de metadatos como ocupados.
7. `initAI()` enlaza todos los inodos como libres.
8. `reservar_inodo('d', 7)` reserva el inodo raíz (tipo directorio, permisos rwx).
9. Actualiza `posInodoRaiz = 0` en el SB.
10. `bumount()` cierra el disco.

### Intuición

Llenar el disco de ceros antes de inicializar las estructuras garantiza que no haya datos residuales de un formateo anterior. La heurística `ninodos = nbloques / 4` asume que cada inodo gestionará ~4 bloques de datos en promedio.

### Riesgos y edge cases

- **`nbloques` muy pequeño:** Si `nbloques < 4`, `ninodos = 0` y el sistema no tiene inodos. No hay validación explícita.
- **Disco ya existente:** `bmount` con `O_CREAT` no trunca el fichero existente. El bucle de ceros sobrescribe los bloques antiguos, pero si el fichero era más grande que `nbloques * BLOCKSIZE`, los bytes sobrantes permanecen.

### Resumen técnico final

`mi_mkfs` es el único programa que puede crear un disco virtual válido desde cero. Sin él, no hay sistema de ficheros.

---

## 3.2 `mi_ls.c`

### Introducción del módulo

Lista el contenido de un directorio o muestra información de un fichero. Equivale a `ls` de Unix, con soporte para `-l` (listado detallado).

### Explicación detallada

1. Parsea argumentos: `mi_ls [-l] <disco> <ruta>`.
2. Determina `tipo`: `'d'` si la ruta termina en `/`, `'f'` si no.
3. `bmount`, llama a `mi_dir(camino, buffer, tipo)`.
4. Si éxito, imprime cabecera y el buffer formateado.
5. `bumount`.

### Intuición

El carácter `/` al final de la ruta es la convención del proyecto para distinguir directorios de ficheros. Esto evita tener que consultar el inodo antes de decidir cómo tratar la ruta.

### Riesgos y edge cases

- **Ruta vacía o `/`:** Si `camino` es `/`, `strlen(camino)-1` accede al carácter `/`, que es correcto. Pero si `camino` es cadena vacía, `strlen(camino)-1` es underflow.

### Resumen técnico final

`mi_ls` es la interfaz de consulta del sistema de ficheros. Delega toda la lógica en `mi_dir`.

---

## 3.3 `mi_mkdir.c`

### Introducción del módulo

Crea un directorio con permisos especificados. Equivale a `mkdir`.

### Explicación detallada

1. Valida: `mi_mkdir <disco> <permisos> <ruta/>`.
2. Verifica permisos en rango 0–7.
3. Verifica que la ruta termina en `/`.
4. `bmount`, llama a `mi_creat(camino, permisos)`.
5. `bumount`.

### Intuición

La validación de `/` final es redundante con la de `mi_creat` (que llama a `buscar_entrada`), pero proporciona un mensaje de error más específico al usuario.

### Riesgos y edge cases

- **Permisos > 7:** Se validan, pero `atoi` acepta cualquier número. Si el usuario pasa `10`, se rechaza correctamente.

### Resumen técnico final

`mi_mkdir` es un wrapper mínimo sobre `mi_creat` con validación de entrada.

---

## 3.4 `mi_touch.c`

### Introducción del módulo

Crea un fichero vacío con permisos especificados. Equivale a `touch`.

### Explicación detallada

Idéntico a `mi_mkdir` pero verifica que la ruta **NO** termina en `/`. Llama a `mi_creat`.

### Intuición

La simetría con `mi_mkdir` (uno exige `/`, el otro lo prohíbe) refuerza la convención de tipos por ruta.

### Riesgos y edge cases

- Mismos que `mi_mkdir`.

### Resumen técnico final

`mi_touch` es el gemelo de `mi_mkdir` para ficheros.

---

## 3.5 `mi_chmod.c`

### Introducción del módulo

Cambia los permisos de un fichero o directorio existente. Equivale a `chmod`.

### Explicación detallada

1. Valida: `mi_chmod <disco> <permisos> <ruta>`.
2. Verifica permisos 0–7.
3. `bmount`, llama a `mi_chmod(camino, permisos)`.
4. `bumount`.

### Resumen técnico final

Wrapper mínimo sobre `mi_chmod` de `directorios.c`.

---

## 3.6 `mi_link.c`

### Introducción del módulo

Crea un enlace duro (hard link) entre dos rutas. Equivale a `ln`.

### Explicación detallada

1. Valida: `mi_link <disco> <ruta_origen> <ruta_enlace>`.
2. Verifica que ninguna ruta termina en `/` (no se permiten enlaces a directorios).
3. `bmount`, llama a `mi_link(ruta_original, ruta_enlace)`.
4. `bumount`.

### Intuición

Prohibir enlaces a directorios evita ciclos en el árbol de directorios, lo cual complicaría la resolución de rutas y el cálculo de espacio usado.

### Resumen técnico final

`mi_link` crea una segunda entrada de directorio apuntando al mismo inodo, incrementando `nlinks`.

---

## 3.7 `mi_rm.c`

### Introducción del módulo

Borra un fichero o directorio. Equivale a `rm`.

### Explicación detallada

1. Valida: `mi_rm <disco> <ruta>`.
2. Protege la raíz: `strcmp(camino, "/") == 0` → error.
3. `bmount`, llama a `mi_unlink(camino)`.
4. `bumount`.

### Intuición

Proteger la raíz evita que el usuario deje el sistema de ficheros sin punto de entrada, lo cual lo haría inaccesible.

### Resumen técnico final

`mi_rm` delega en `mi_unlink`, que gestiona la eliminación de entradas y liberación de inodos.

---

## 3.8 `mi_rmdir.c`

### Introducción del módulo

Borra un directorio vacío. Equivale a `rmdir`.

### Explicación detallada

1. Valida: `mi_rmdir <disco> <ruta/>`.
2. Verifica que la ruta termina en `/`.
3. `bmount`, llama a `mi_unlink(camino)`.
4. Si error es `ERROR_ENTRADA_YA_EXISTENTE` (reutilizado para "directorio no vacío"), imprime mensaje personalizado.
5. `bumount`.

### Intuición

Reutilizar `ERROR_ENTRADA_YA_EXISTENTE` para "directorio no vacío" es un hack: el código de error no semánticamente correcto pero funciona porque `mi_unlink` retorna ese código cuando el directorio tiene entradas.

### Resumen técnico final

`mi_rmdir` es `mi_rm` con validación de `/` y mensaje personalizado para directorios no vacíos.

---

## 3.9 `mi_escribir.c`

### Introducción del módulo

Escribe texto en un fichero a partir de un offset. Si el fichero no existe, se crea.

### Explicación detallada

1. Valida: `mi_escribir <disco> <ruta> <texto> <offset>`.
2. Verifica que la ruta no termina en `/`.
3. `bmount`, llama a `mi_write(camino, texto, offset, strlen(texto))`.
4. Imprime bytes escritos.
5. `bumount`.

### Intuición

Permitir offset arbitrario permite sobrescribir datos existentes o crear ficheros sparse (escribir en offset grande sin llenar los anteriores).

### Resumen técnico final

`mi_escribir` es la interfaz de escritura por ruta con offset.

---

## 3.10 `mi_cat.c`

### Introducción del módulo

Muestra el contenido completo de un fichero por stdout. Equivale a `cat`.

### Explicación detallada

1. Valida: `mi_cat <disco> <ruta>`.
2. Buffer de 1500 bytes (fuerza lecturas fragmentadas: 1500 > 1024).
3. Bucle: `mi_read(camino, buffer, offset, 1500)` hasta que retorne ≤ 0.
4. `write(1, buffer, leidos)` vuelca cada chunk a stdout.
5. `bumount`.

### Intuición

El buffer de 1500 bytes es deliberadamente no múltiplo de 1024 para demostrar que `mi_read` maneja correctamente lecturas que cruzan límites de bloque.

### Resumen técnico final

`mi_cat` lee secuencialmente y vuelca a stdout. El buffer de 1500 bytes prueba la fragmentación.

---

## 3.11 `mi_stat.c`

### Introducción del módulo

Muestra los metadatos de un fichero o directorio. Equivale a `stat`.

### Explicación detallada

1. Valida: `mi_stat <disco> <ruta>`.
2. `bmount`, llama a `mi_stat(camino, &stat)`.
3. Formatea timestamps con `localtime` y `sprintf`.
4. Imprime: ninodo, tipo, permisos, atime, mtime, ctime, nlinks, tamaño, bloques.
5. `bumount`.

### Resumen técnico final

`mi_stat` es la interfaz de consulta de metadatos por ruta.

---

# 4. Utilidades

## 4.1 `leer_sf.c`

### Introducción del módulo

Herramienta de diagnóstico que lee y muestra el superbloque, inodos individuales, y el bitmap. Esencial para depurar el estado interno del sistema de ficheros.

### Explicación detallada

**`mostrar_inodo(inodo, ninodo)`:** Formatea y muestra todos los campos de un inodo, convirtiendo `time_t` a cadenas legibles con `localtime` y `strftime`.

**`main`:**
1. `bmount(argv[1])`.
2. Lee el superbloque con `bread(posSB, &SB)`.
3. Imprime todas las posiciones y contadores del SB.
4. Tiene bloques `#if` comentados para debug de distintos niveles (mostrar inodos, bitmap, etc.).
5. `bumount`.

### Intuición

`leer_sf` es el equivalente a `debugfs` o `dumpe2fs` en sistemas reales. Sin esta herramienta, depurar errores de asignación de bloques o inodos sería extremadamente difícil.

### Riesgos y edge cases

- Los bloques `#if` comentados requieren definir `DEBUGN2`, etc. en el Makefile para activarse. Si se olvidan, el código de debug no se compila.

### Resumen técnico final

`leer_sf` es la herramienta de inspección del estado interno del FS. Indispensable para debugging.

---

## 4.2 `escribir.c`

### Introducción del módulo

Programa de test que escribe texto en offsets predefinidos para forzar la activación de todos los niveles de punteros (directos, indirecto simple, doble, triple).

### Explicación detallada

**Offsets de test:**
| Offset | Bloque Lógico | Nivel de Puntero |
|--------|--------------|------------------|
| 9000 | 8 | Directo |
| 209000 | 204 | Directo (cruza bloque) |
| 30.725.000 | 30.004 | Indirecto Simple |
| 409.605.000 | 400.004 | Indirecto Doble |
| 480.000.000 | 468.750 | Indirecto Triple |

**Modos:**
- `diferentes_inodos = 0`: Todas las escrituras en el mismo inodo.
- `diferentes_inodos = 1`: Cada escritura en un inodo diferente.

Tras cada escritura, llama a `mi_stat_f` para verificar el estado del inodo.

### Intuición

Los offsets están calculados para caer justo en los límites de cada nivel de indirección. Esto garantiza que `traducir_bloque_inodo` recorra todas las ramas del árbol.

### Riesgos y edge cases

- **Disco insuficiente:** Si el disco es pequeño (< 500 MB), los offsets de indirecto doble/triple fallarán por falta de bloques.
- **`diferentes_inodos = 1` consume muchos inodos:** 5 inodos por ejecución. Si se ejecuta muchas veces sin liberar, se agotan los inodos.

### Resumen técnico final

`escribir.c` es el test de integración del direccionamiento multinivel. Verifica que todos los niveles de punteros funcionan correctamente.

---

## 4.3 `leer.c`

### Introducción del módulo

Lee secuencialmente todo el contenido de un fichero por número de inodo y lo vuelca a stdout.

### Explicación detallada

1. `bmount`, buffer de `BLOCKSIZE * 4` (4096 bytes).
2. Bucle: `mi_read_f(ninodo, buffer, offset, 4096)` hasta EOF.
3. `write(1, buffer, leidos)` a stdout.
4. Verificación final con `mi_stat_f` para comparar bytes leídos con `tamEnBytesLog`.

### Intuición

Usar `mi_read_f` directamente (por inodo) en lugar de `mi_read` (por ruta) permite leer ficheros sin entrada de directorio, útil para debugging.

### Resumen técnico final

`leer.c` es el complemento de `escribir.c`: lee por inodo y verifica coherencia con stat.

---

## 4.4 `truncar.c`

### Introducción del módulo

Trunca un fichero a un tamaño dado por número de inodo. Si `nbytes = 0`, libera completamente el inodo.

### Explicación detallada

1. `bmount`.
2. Si `nbytes == 0`: llama a `liberar_inodo(ninodo)` (eliminación completa).
3. Si `nbytes > 0`: llama a `mi_truncar_f(ninodo, nbytes)` (truncado parcial).
4. Muestra estado final con `mi_stat_f`.
5. `bumount`.

### Intuición

La bifurcación `nbytes == 0` vs `nbytes > 0` permite probar tanto la liberación completa como el truncado parcial con un solo programa.

### Riesgos y edge cases

- **`liberar_inodo` directo:** Si el inodo tiene `nlinks > 1`, `liberar_inodo` lo marca como libre pero las otras entradas de directorio siguen apuntando a él. Esto corrompe el sistema. En uso normal, `mi_unlink` gestiona esto correctamente.

### Resumen técnico final

`truncar.c` prueba la liberación de bloques y inodos. Útil para verificar que `mi_truncar_f` y `liberar_inodo` funcionan correctamente.

---

## 4.5 `permitir.c`

### Introducción del módulo

Cambia los permisos de un inodo por número. Equivale a `chmod` pero por inodo en lugar de por ruta.

### Explicación detallada

1. `bmount`.
2. `mi_chmod_f(ninodo, permisos)`.
3. `bumount`.

### Resumen técnico final

`permitir.c` es un wrapper mínimo sobre `mi_chmod_f` para testing directo por inodo.

---

## 4.6 `prueba_nivel7.c`

### Introducción del módulo

Test harness de integración para el Nivel 7: resolución de caminos y gestión de entradas de directorio.

### Explicación detallada

**Fases del test:**
1. **Creación:** Crea `/docs`, `/docs/trabajo`, `/docs/trabajo/informe.txt` usando `buscar_entrada` con `reservar=1`.
2. **Búsqueda:** Busca entradas existentes y no existentes con `reservar=0`.
3. **Duplicados:** Intenta crear entradas que ya existen, verificando que se retorna `ERROR_ENTRADA_YA_EXISTENTE`.

### Intuición

Este test verifica que `buscar_entrada` funciona correctamente en los 3 escenarios: crear nuevo, encontrar existente, y rechazar duplicado.

### Resumen técnico final

`prueba_nivel7.c` es el test de integración de la resolución de caminos. Cubre los casos principales de `buscar_entrada`.

---

# 5. Concurrencia

## 5.1 `simulacion.c`

### Introducción del módulo

Simula 100 procesos escribiendo concurrentemente en el sistema de ficheros. Cada proceso crea su propio directorio y fichero `prueba.dat`, y escribe 50 registros en posiciones aleatorias con delays de 50ms.

### Explicación detallada

**Enterrador de zombies:**
```c
void reaper() {
    pid_t ended;
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0) {
        acabados++;
    }
}
```
Se registra con `signal(SIGCHLD, reaper)`. Cada vez que un hijo termina, `waitpid(-1, NULL, WNOHANG)` lo reap sin bloquear. El bucle `while` maneja múltiples hijos que terminan simultáneamente.

**Flujo del padre:**
1. `bmount` y crea directorio `/simul_aaaammddhhmmss/`.
2. Bucle de 100 `fork()`:
   - Espera 150ms entre forks para escalonar la creación.
3. `while (acabados < NUMPROCESOS) pause()`: espera señales SIGCHLD hasta que todos los hijos terminen.
4. `bumount`.

**Flujo de cada hijo:**
1. `bmount` (remonta para tener descriptor propio).
2. Crea `/simul_.../proceso_PID/`.
3. Crea `/simul_.../proceso_PID/prueba.dat`.
4. Bucle de 50 escrituras:
   - Crea `struct REGISTRO` con fecha, PID, número de escritura, y posición aleatoria (`rand() % REGMAX`).
   - `mi_write(camino_fichero, &registro, registro.nRegistro * sizeof(REGISTRO), sizeof(REGISTRO))`.
   - `usleep(50000)` (50ms de delay).
5. `bumount` y `exit(0)` (dispara SIGCHLD).

### Intuición

El remontaje en cada hijo (`bmount` tras `fork`) es esencial porque el descriptor de fichero se comparte entre padre e hijos, incluyendo el puntero de `lseek`. Cada hijo necesita su propio descriptor independiente.

El delay de 50ms entre escrituras y 150ms entre forks asegura que las escrituras de diferentes procesos se solapen temporalmente, probando la efectividad del semáforo.

### Riesgos y edge cases

- **`srand(time(NULL) + getpid())`:** Si dos hijos se fork-ean en el mismo segundo, `time(NULL)` es igual pero `getpid()` difiere, asegurando semillas diferentes.
- **`pause()` puede retornar por otras señales:** Si el proceso recibe una señal no manejada, `pause()` retorna y el bucle continúa. En la práctica, solo SIGCHLD está registrado.
- **Semáforo POSIX compartido:** Todos los procesos comparten `/sem_mi_sf`. Si otro programa usa el mismo nombre, las escrituras se serializan con ese programa también.

### Resumen técnico final

`simulacion.c` es el test de concurrencia del sistema. Fork-ear 100 procesos que escriben simultáneamente verifica que el semáforo protege correctamente contra corrupciones de datos.

---

## 5.2 `verificacion.c`

### Introducción del módulo

Verifica la coherencia de la simulación de concurrencia. Recorre los directorios de cada proceso, lee sus `prueba.dat`, verifica que los registros son consistentes, y genera `informe.txt`.

### Explicación detallada

**Flujo:**
1. `bmount` y verifica que existe el directorio de simulación.
2. Crea `informe.txt` dentro del FS.
3. Lee todas las entradas del directorio de simulación con `mi_read`.
4. Para cada entrada `proceso_PID`:
   - Extrae el PID del nombre.
   - Lee `prueba.dat` secuencialmente en bloques de 256 registros (6144 bytes).
   - Para cada registro: verifica que `reg.pid == pid` (coherencia).
   - Rastrea: primera escritura, última escritura, menor posición, mayor posición.
   - Genera texto formateado y lo añade a `informe.txt` con `mi_write` (append).
5. `free(entradas)`, `bumount`.

**Criterios de verificación:**
- **PID consistente:** Cada registro debe tener el PID del proceso propietario.
- **Primera/Última escritura:** Menor/mayor `nEscritura`, desempate por timestamp.
- **Menor/Mayor posición:** Menor/mayor `nRegistro`.

### Intuición

Leer en bloques de 256 registros (múltiplo de `sizeof(REGISTRO)`) garantiza que cada lectura contiene registros completos, sin cortes a mitad de estructura. El append a `informe.txt` se hace obteniendo el tamaño actual con `mi_stat` y escribiendo al final.

### Riesgos y edge cases

- **`malloc(stat_dir.tamEnBytesLog)`:** Si el directorio es muy grande, la asignación puede fallar. No hay verificación de `malloc` returning NULL.
- **Registros duplicados:** Si el semáforo falla, un proceso podría sobrescribir el registro de otro. La verificación de PID detecta esto (el registro tendría un PID diferente al esperado).
- **Registros perdidos:** Si una escritura se pierde, `nEscrituras` será menor que `NUMESCRITURAS`. El informe lo muestra claramente.

### Resumen técnico final

`verificacion.c` es el validador de la simulación. Genera un informe detallado por proceso que permite verificar que la concurrencia se gestionó correctamente.

---

# Apéndice: Mapa de Dependencias entre Módulos

```
semaforo_mutex_posix.h/c
        ↑
    bloques.h/c  (bmount, bumount, bread, bwrite, mi_waitSem, mi_signalSem)
        ↑
ficheros_basico.h/c  (initSB, initMB, initAI, reservar/liberar_bloque/inodo, traducir_bloque_inodo)
        ↑
    ficheros.h/c  (mi_write_f, mi_read_f, mi_stat_f, mi_chmod_f, mi_truncar_f)
        ↑
directorios.h/c  (extraer_camino, buscar_entrada, mi_creat, mi_dir, mi_write, mi_read, mi_link, mi_unlink)
        ↑
┌───────┴───────┬───────────────┬───────────────┬──────────────┐
│  Comandos     │  Utilidades   │ Concurrencia  │  Test        │
│  mi_mkfs      │  leer_sf      │ simulacion    │ prueba_nivel7│
│  mi_ls        │  escribir     │ verificacion  │              │
│  mi_mkdir     │  leer         │               │              │
│  mi_touch     │  truncar      │               │              │
│  mi_chmod     │  permitir     │               │              │
│  mi_link      │               │               │              │
│  mi_rm        │               │               │              │
│  mi_rmdir     │               │               │              │
│  mi_escribir  │               │               │              │
│  mi_cat       │               │               │              │
│  mi_stat      │               │               │              │
└───────────────┴───────────────┴───────────────┴──────────────┘
```

---

*Documentación generada a partir del análisis directo de cada fichero fuente del proyecto.*
