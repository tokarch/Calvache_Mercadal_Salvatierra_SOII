# Documentacion de Funcionamiento Interno

Sistema de ficheros basado en inodos implementado en C sobre un disco virtual. Proyecto para la asignatura Sistemas Operativos II (SOII).

---

## 1. Ciclo de vida del programa

### 1.1. Comandos de usuario (mi_ls, mi_cat, mi_mkdir, mi_rm, mi_touch, mi_chmod, mi_stat, mi_escribir, mi_link)

Todos los comandos de usuario siguen el mismo patron de vida:

1. **Parseo de argumentos**: Se valida `argc` y se extraen los parametros con `argv[]`. Los valores numericos se convierten con `atoi`, las banderas con `strcmp`.
2. **Montaje del disco**: `bmount(nombre_dispositivo)` abre el fichero de disco virtual y obtiene un descriptor de fichero.
3. **Operacion principal**: Se ejecuta la logica propia del comando (listar, leer, escribir, crear, borrar, cambiar permisos, obtener estadisticas).
4. **Desmontaje**: `bumount()` cierra el descriptor del fichero y libera el semaforo POSIX.

Cada comando es un proceso independiente que monta y desmonta el disco en cada ejecucion. No hay un demonio residente. Esto implica que cada invocacion tiene su propio espacio de memoria, su propio descriptor de fichero y su propia copia del superbloque en memoria.

### 1.2. Ciclo de vida de mi_mkfs (formateador del sistema de ficheros)

`mi_mkfs` es el programa que crea un sistema de ficheros desde cero sobre un fichero de disco. Su secuencia es:

1. **Validacion de argumentos**: Se esperan exactamente 3 argumentos (`argv[0]="mi_mkfs"`, `argv[1]=nombre_dispositivo`, `argv[2]=nbloques`). Si `argc != 3`, se imprime un mensaje de error en `stderr` y se retorna `FALLO`.

2. **Montaje del disco**: `bmount(nombre_dispositivo)` abre o crea el fichero de disco con `open(O_RDWR | O_CREAT, 0666)`.

3. **Escritura de ceros**: Se declara un buffer de `BLOCKSIZE` (1024) bytes y se llena de ceros con `memset(buffer, 0, BLOCKSIZE)`. Luego se itera sobre todos los bloques del disco, escribiendo el buffer de ceros en cada uno mediante `bwrite(i, buffer)`. Esto garantiza que el disco este completamente limpio, sin datos residuales de usos anteriores.

4. **Validacion del tamano del inodo**: Se comprueba que `sizeof(struct inodo) == 128`. Si la estructura no mide exactamente 128 bytes, el formateo se aborta. Esta comprobacion evita corrupciones silenciosas si se modifica la estructura del inodo.

5. **Inicializacion del Superbloque (`initSB`)**: Se calculan las posiciones absolutas de cada zona del disco:
   - El superbloque siempre ocupa el bloque 0 (`posSB = 0`).
   - El Mapa de Bits comienza en el bloque 1 (`posPrimerBloqueMB = posSB + tamSB`).
   - El Array de Inodos comienza despues del Mapa de Bits.
   - La Zona de Datos comienza despues del Array de Inodos.
   - Se inicializan los contadores: todos los bloques e inodos se marcan como libres inicialmente.
   - El superbloque se escribe en el bloque 0 con `bwrite(posSB, &SB)`.

6. **Inicializacion del Mapa de Bits (`initMB`)**: Se leen los bloques del Mapa de Bits y se marcan como ocupados (bit a 1) todos los bloques que corresponden a metadatos (Superbloque + Mapa de Bits + Array de Inodos). El resto de bits quedan a 0 (bloques libres). Se usa `memset(buffer, 255, bytes)` para marcar bytes completos de forma eficiente, y se calcula la mascara de bits restantes con `(0xFF << (8 - bitsRestantes))`. Finalmente se actualiza `SB.cantBloquesLibres` restando los bloques de metadatos.

7. **Inicializacion del Array de Inodos (`initAI`)**: Se recorren todos los bloques del Array de Inodos. Cada inodo se marca como libre (`tipo = 'l'`) y se enlaza formando una lista enlazada mediante `punterosDirectos[0]`. El inodo `i` apunta al inodo `i+1`, y el ultimo inodo apunta a `UINT_MAX` (fin de lista). Esta lista enlazada permite que `reservar_inodo` obtenga el siguiente inodo libre en tiempo constante.

8. **Reserva del inodo raiz (`reservar_inodo`)**: Se reserva el inodo 0 para el directorio raiz del sistema de ficheros. Se inicializa con `tipo = 'd'` (directorio), `permisos = 7` (rwx), `nlinks = 1`, y todas las fechas (`atime`, `mtime`, `ctime`, `btime`) al momento actual. Los punteros se ponen a cero con `memset`.

9. **Desmontaje**: `bumount()` cierra el descriptor del fichero y destruye el semaforo POSIX.

### 1.3. Ciclo de vida de simulacion (concurrencia con 100 procesos)

`simulacion.c` es el programa que demuestra el funcionamiento concurrente del sistema de ficheros. Su ciclo de vida es el mas complejo del proyecto:

1. **Validacion de argumentos**: Se espera un solo argumento, el nombre del disco (`argc != 2` implica error).

2. **Registro del manejador de senales**: `signal(SIGCHLD, reaper)` asocia la senal `SIGCHLD` a la funcion `reaper`. Cada vez que un proceso hijo termina, el kernel envia `SIGCHLD` al padre, y `reaper` se ejecuta de forma asincrona.

3. **Montaje del disco (padre)**: `bmount(argv[1])` monta el disco para el proceso padre.

4. **Creacion del directorio de simulacion**: Se genera un nombre de directorio con la marca de tiempo actual usando `time(NULL)` y `localtime`, con formato `/simul_aaaammddhhmmss/`. Se crea con `mi_creat(camino_simul, 6)`.

5. **Bucle de creacion de 100 procesos hijos**: Se itera 100 veces (`NUMPROCESOS`). En cada iteracion:
   - `fork()` crea un proceso hijo.
   - **Proceso hijo** (`pid == 0`):
     - **Remonta el disco** con `bmount(argv[1])`. Esto es necesario porque el descriptor heredado del padre comparte el puntero de posicion del fichero (`lseek`). Si el hijo usara el descriptor heredado, sus operaciones de `lseek` interferirian con las del padre y de otros hermanos. `bmount` detecta que `descriptor > 0` y lo cierra antes de abrir uno nuevo, garantizando que cada hijo tenga su propio descriptor independiente.
     - Crea su directorio personal `/simul_.../proceso_PID/` con `mi_creat`.
     - Crea el fichero `prueba.dat` dentro de su directorio.
     - Inicializa la semilla aleatoria con `srand(time(NULL) + getpid())`. La combinacion de `time(NULL)` y `getpid()` asegura que cada hijo tenga una secuencia aleatoria diferente, incluso si se crean en el mismo segundo.
     - Ejecuta 50 escrituras (`NUMESCRITURAS`): cada una crea un `struct REGISTRO` con la marca de tiempo en microsegundos (`gettimeofday`), el PID, el numero de escritura y una posicion aleatoria (`rand() % REGMAX`). El registro se escribe en el fichero con `mi_write` en la posicion `nRegistro * sizeof(struct REGISTRO)`.
     - Entre escritura y escritura, el hijo duerme 50000 microsegundos con `usleep(50000)`.
     - Al terminar, llama a `bumount()` y `exit(0)`. El `exit(0)` genera la senal `SIGCHLD` que el padre recibira.
   - **Proceso padre**: Despues de cada `fork`, espera 150000 microsegundos con `usleep(150000)` antes de crear el siguiente hijo. Este retardo escalona la creacion de procesos, evitando que los 100 hijos se lancen simultaneamente y reduzcan la contencion excesiva en el semaforo.

6. **Espera de finalizacion**: El padre entra en un bucle `while (acabados < NUMPROCESOS)` con `pause()`. `pause()` bloquea al proceso hasta que recibe una senal. Cuando un hijo termina, se dispara `SIGCHLD`, se ejecuta `reaper`, y `pause()` retorna. El contador `acabados` se incrementa por cada hijo reaperado.

7. **Manejador `reaper`**: La funcion `reaper` se ejecuta en contexto de senal. Usa `waitpid(-1, NULL, WNOHANG)` en un bucle `while`. El `-1` indica "cualquier hijo", `NULL` descarta el estado de salida, y `WNOHANG` es crucial: hace que `waitpid` retorne inmediatamente si no hay hijos terminados, en lugar de bloquear. Esto permite que un solo `SIGCHLD` reapare multiples hijos que hayan terminado casi simultaneamente. Sin `WNOHANG`, el manejador se bloquearia en el primer hijo y los demas quedarian como zombies hasta la siguiente senal.

8. **Desmontaje final**: Cuando los 100 hijos han terminado, el padre llama a `bumount()` y retorna `EXITO`.

---

## 2. Parseo de argumentos

Todos los programas del proyecto siguen un patron comun de validacion de argumentos de linea de comandos.

### 2.1. Validacion de argc

Cada programa verifica que el numero de argumentos sea el esperado antes de acceder a `argv[]`:

```c
// mi_mkfs: necesita nombre_dispositivo y nbloques
if (argc != 3) {
    fprintf(stderr, "Sintaxis incorrecta: %s <nombre_dispositivo> <nbloques>\n", argv[0]);
    return FALLO;
}

// mi_cat: necesita disco y ruta
if (argc != 3) {
    fprintf(stderr, "Sintaxis: %s <disco> <ruta_fichero>\n", argv[0]);
    return FALLO;
}

// mi_mkdir: necesita disco, permisos y ruta
if (argc != 4) {
    fprintf(stderr, "Sintaxis: ./mi_mkdir <nombre_dispositivo> <permisos> </ruta_directorio/>.\n");
    return FALLO;
}
```

### 2.2. Conversion de argumentos numericos

Los argumentos numericos se convierten con `atoi`:

```c
// mi_mkfs: nbloques como entero
int nbloques = atoi(argv[2]);

// mi_mkdir: permisos en octal (0-7)
int permisos = atoi(argv[2]);
if (permisos < 0 || permisos > 7) {
    fprintf(stderr, "Error: modo invalido: <<%d>>\n", permisos);
    return FALLO;
}
```

La validacion de permisos en `mi_mkdir` es basica: solo acepta valores entre 0 y 7. Esto corresponde a un unico digito octal que representa los permisos combinados (r=4, w=2, x=1). Por ejemplo, 6 significa rw (lectura y escritura), 7 significa rwx (todos los permisos).

### 2.3. Parseo de banderas (-l en mi_ls)

`mi_ls` es el unico comando que soporta una bandera opcional `-l` para listado detallado. El parseo usa `strcmp` y un indice desplazable:

```c
int arg_idx = 1;
if (argc == 4 && strcmp(argv[1], "-l") == 0) {
    arg_idx = 2;  // La bandera ocupa argv[1], los argumentos reales empiezan en argv[2]
} else if (argc != 3) {
    fprintf(stderr, "Sintaxis: %s [-l] <disco> <ruta>\n", argv[0]);
    return FALLO;
}
char *nombre_dispositivo = argv[arg_idx];
char *camino = argv[arg_idx + 1];
```

Este patron permite que `arg_idx` apunte siempre al primer argumento posicional, independientemente de si se uso `-l` o no.

### 2.4. Validacion de rutas: directorios vs ficheros

El sistema distingue directorios de ficheros por la presencia del caracter `/` al final de la ruta. Esta convencion es fundamental para el funcionamiento de `buscar_entrada`:

```c
// mi_ls: determina el tipo por el ultimo caracter
char tipo = 'd';  // Por defecto directorio
if (camino[strlen(camino)-1] != '/') {
    tipo = 'f';  // Si no termina en '/', es fichero
}

// mi_cat: rechaza rutas que terminan en '/'
if (camino[strlen(camino) - 1] == '/') {
    fprintf(stderr, "Error: La ruta corresponde a un directorio.\n");
    return FALLO;
}

// mi_mkdir: exige que la ruta termine en '/'
if (camino[strlen(camino)-1] != '/') {
    fprintf(stderr, "Error: Camino incorrecto.\n");
    return FALLO;
}
```

Esta convencion tiene una razon de ser profunda: `extraer_camino` y `buscar_entrada` usan el `/` final para determinar si el ultimo componente de la ruta es un directorio o un fichero. Si la ruta es `/home/user/`, el ultimo componente `user` se trata como directorio (`tipo = 'd'`). Si la ruta es `/home/user/file.txt`, el ultimo componente se trata como fichero (`tipo = 'f'`).

---

## 3. Acceso al sistema de ficheros

### 3.1. El disco virtual como fichero plano

El sistema de ficheros no opera sobre un dispositivo de bloques real. En su lugar, utiliza un fichero plano (por ejemplo, `disco`) que simula un dispositivo de almacenamiento. Este fichero es una secuencia contigua de bytes donde cada segmento de `BLOCKSIZE` (1024) bytes representa un bloque logico del disco.

La estructura del disco es:

```
Bloque 0:          Superbloque (metadatos globales)
Bloques 1..N:      Mapa de Bits (1 bit por bloque de datos)
Bloques N+1..M:    Array de Inodos (inodos de 128 bytes)
Bloques M+1..fin:  Zona de Datos (contenido de ficheros y directorios)
```

### 3.2. bmount: apertura del disco

```c
int bmount(const char *camino) {
    // Si el descriptor ya esta abierto (heredado tras fork), cerrarlo
    if (descriptor > 0) {
        close(descriptor);
    }

    // Configurar permisos: rw-rw-rw- (0666)
    umask(0000);

    // Abrir el fichero de disco
    descriptor = open(camino, O_RDWR | O_CREAT, 0666);
    if (descriptor == -1) {
        perror("Error en bmount al abrir el fichero");
        return FALLO;
    }

    // Inicializar semaforo si no existe
    if (!mutex) {
        mutex = initSem();
        if (mutex == SEM_FAILED) {
            close(descriptor);
            return FALLO;
        }
    }

    return descriptor;
}
```

`bmount` realiza tres acciones clave:

- **Cierre del descriptor heredado**: Si `descriptor > 0`, significa que el proceso heredo un descriptor abierto de su padre (tras un `fork`). Se cierra para que el hijo obtenga su propio descriptor independiente. Sin este cierre, padre e hijos compartirian el mismo puntero de posicion del fichero, causando condiciones de carrera en `lseek`.

- **`umask(0000)`**: La mascara de creacion de ficheros del sistema operativo puede restringir los permisos que solicitamos. En entornos como WSL, la umask por defecto puede ser `0022`, lo que convertiria los permisos `0666` en `0644` (rw-r--r--). Al establecer `umask(0000)`, nos aseguramos de que el fichero de disco se cree con permisos `rw-rw-rw-` (0666), permitiendo que cualquier usuario pueda leer y escribir en el disco virtual.

- **`open(O_RDWR | O_CREAT, 0666)`**: `O_RDWR` permite lectura y escritura. `O_CREAT` crea el fichero si no existe. Los permisos `0666` se aplican solo en la creacion.

### 3.3. bread y bwrite: lectura y escritura de bloques

```c
int bwrite(unsigned int nbloque, const void *buf) {
    off_t desplazamiento = (off_t)nbloque * BLOCKSIZE;

    if (lseek(descriptor, desplazamiento, SEEK_SET) == (off_t)-1) {
        perror("Error en bwrite al mover el puntero (lseek)");
        return FALLO;
    }

    size_t bytes_escritos = write(descriptor, buf, BLOCKSIZE);
    if (bytes_escritos == -1) {
        perror("Error en bwrite al escribir (write)");
        return FALLO;
    }

    return (int)bytes_escritos;
}

int bread(unsigned int nbloque, void *buf) {
    off_t desplazamiento = (off_t)nbloque * BLOCKSIZE;

    if (lseek(descriptor, desplazamiento, SEEK_SET) == (off_t)-1) {
        perror("Error en bread al mover el puntero (lseek)");
        return FALLO;
    }

    size_t bytes_leidos = read(descriptor, buf, BLOCKSIZE);
    if (bytes_leidos == -1) {
        perror("Error en bread al leer (read)");
        return FALLO;
    }

    return (int)bytes_leidos;
}
```

La traduccion de bloque logico a posicion fisica es directa: el bloque `nbloque` comienza en el byte `nbloque * BLOCKSIZE` del fichero. `lseek` posiciona el puntero del fichero en ese offset absoluto, y `read`/`write` operan exactamente `BLOCKSIZE` bytes.

El cast a `off_t` en `(off_t)nbloque * BLOCKSIZE` evita desbordamientos aritmeticos. Si `nbloque` es un `unsigned int` de 32 bits y `BLOCKSIZE` es 1024, la multiplicacion podria exceder el rango de `int` en discos grandes.

### 3.4. Descriptor estatico en bloques.c

El descriptor de fichero se declara como variable estatica a nivel de fichero:

```c
static int descriptor = 0;
```

La palabra clave `static` en este contexto significa que `descriptor` tiene enlazado interno (internal linkage). Solo es visible dentro de `bloques.c`. Ningun otro fichero fuente puede acceder directamente a esta variable. Esto es un principio de encapsulacion: las capas superiores (ficheros.c, directorios.c) no necesitan conocer el descriptor, solo usan `bread` y `bwrite`.

La consecuencia practica es que cada proceso que carga el codigo de `bloques.c` tiene su propia copia de `descriptor`. Tras un `fork`, el hijo hereda una copia del descriptor con el mismo valor numerico, pero apunta al mismo fichero abierto en el kernel. Por eso `bmount` cierra el descriptor heredado y abre uno nuevo: para que cada proceso tenga su propia entrada en la tabla de ficheros abiertos y su propio puntero de posicion independiente.

---

## 4. Uso de llamadas POSIX en el contexto del proyecto

### 4.1. open, read, write, close, lseek

Estas cinco llamadas son la base de toda operacion de E/S en el proyecto. Se usan exclusivamente en `bloques.c` para implementar `bmount`, `bumount`, `bread` y `bwrite`.

**`open`**: Se usa en `bmount` para abrir o crear el fichero de disco. Los flags `O_RDWR | O_CREAT` indican que se necesita lectura y escritura, y que el fichero se crea si no existe. Los permisos `0666` se aplican en la creacion, modulados por la `umask`.

**`lseek`**: Es la pieza clave que traduce bloques logicos a posiciones fisicas. Cada llamada a `bread` o `bwrite` calcula `desplazamiento = nbloque * BLOCKSIZE` y posiciona el puntero del fichero en ese offset. `SEEK_SET` indica que el offset es absoluto desde el inicio del fichero. Sin `lseek`, no podriamos acceder a bloques arbitrarios del disco, solo leer/escribir secuencialmente.

**`read` y `write`**: Operan siempre con exactamente `BLOCKSIZE` bytes. `read` lee un bloque completo del disco en un buffer de memoria. `write` escribe un bloque completo desde memoria al disco. La garantia de que se leen/escriben exactamente 1024 bytes es fundamental para la integridad del sistema de ficheros.

**`close`**: Se usa en `bumount` para liberar el descriptor del fichero. Tambien se usa en `bmount` cuando se detecta un descriptor heredado tras un `fork`.

### 4.2. fork, exit, waitpid

`simulacion.c` es el unico programa que usa estas llamadas, y lo hace para demostrar la concurrencia del sistema de ficheros.

**`fork`**: Crea 100 procesos hijos. Cada hijo ejecuta el mismo codigo que el padre (no hay `exec`). La razon para no usar `exec` es que los hijos deben ejecutar la misma logica de escritura de registros que esta codificada en el propio `simulacion.c`. No necesitan reemplazar su imagen de proceso con otro programa.

```c
for (int proceso = 1; proceso <= NUMPROCESOS; proceso++) {
    pid_t pid = fork();
    if (pid == 0) {
        // Codigo del hijo
        bmount(argv[1]);  // Remontar para descriptor propio
        // ... escribir 50 registros ...
        bumount();
        exit(0);
    }
    usleep(150000);  // Padre espera antes de crear el siguiente hijo
}
```

**`exit(0)`**: Cada hijo llama a `exit(0)` al terminar. Esto libera todos los recursos del proceso y envia `SIGCHLD` al padre. El argumento `0` indica terminacion exitosa.

**`waitpid(-1, NULL, WNOHANG)`**: Se usa dentro del manejador de senal `reaper`. Los tres argumentos tienen un proposito especifico:
- `-1`: Esperar a cualquier hijo, no a uno en particular.
- `NULL`: No nos interesa el estado de salida del hijo, solo queremos reaparearlo.
- `WNOHANG`: No bloquear. Si no hay hijos terminados, retornar inmediatamente con 0.

`WNOHANG` es esencial porque el manejador de senal se ejecuta en contexto asincrono. Si `waitpid` bloqueara, el manejador no retornaria, y el padre quedaria atrapado dentro de `reaper` en lugar de continuar con `pause()`. Ademas, un solo `SIGCHLD` puede corresponder a multiples hijos terminados simultaneamente. El bucle `while` con `WNOHANG` permite reaparelos todos en una sola invocacion del manejador.

### 4.3. signal

```c
signal(SIGCHLD, reaper);
```

`signal` registra `reaper` como manejador de la senal `SIGCHLD`. Cada vez que un hijo termina, el kernel envia esta senal al padre. El manejador se ejecuta de forma asincrona, interrumpiendo cualquier operacion que el padre estuviera realizando (normalmente `pause()`).

La eleccion de `signal` en lugar de `sigaction` es una decision pedagogica del proyecto. `sigaction` es mas portable y tiene un comportamiento mas definido, pero `signal` es suficiente para este caso de uso educativo.

### 4.4. umask

```c
umask(0000);
```

`umask` establece la mascara de creacion de ficheros del proceso. Se llama en `bmount` antes de `open`. El valor `0000` significa "no restringir ningun permiso". Sin esta llamada, la umask heredada del shell (tipicamente `0022`) restringiria los permisos del fichero de disco a `0644` en lugar de `0666`.

Esto es importante porque el disco virtual puede ser accedido por multiples usuarios o procesos. Si los permisos fueran `0644`, solo el propietario podria escribir, y otros procesos fallarian al intentar montar el disco en modo escritura.

### 4.5. time, localtime, strftime

Las funciones de tiempo se usan en todo el proyecto para gestionar las marcas de tiempo de los inodos:

**`time(NULL)`**: Devuelve el tiempo actual en segundos desde epoch (1 de enero de 1970). Se usa en `reservar_inodo` para inicializar las fechas del inodo (`atime`, `mtime`, `ctime`, `btime`), y en `mi_write_f`/`mi_read_f` para actualizar `mtime` y `atime` respectivamente.

**`localtime`**: Convierte un `time_t` a `struct tm` con la hora local. Se usa en `mi_dir` para formatear la fecha de modificacion en la salida de `mi_ls`:

```c
tm = localtime(&inodo.mtime);
sprintf(fecha, "%d-%02d-%02d %02d:%02d:%02d",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);
```

Tambien se usa en `simulacion.c` para generar el nombre del directorio de simulacion con formato `simul_aaaammddhhmmss`.

### 4.6. memset, memcpy, memcmp, strcpy, strlen, strcat

Las funciones de manipulacion de memoria son omnipresentes en el proyecto:

**`memset`**: Se usa para inicializar buffers a cero o a un valor especifico:
- `memset(buffer, 0, BLOCKSIZE)` en `mi_mkfs` para crear un buffer de ceros que formatea el disco.
- `memset(buffer, 255, BLOCKSIZE)` en `reservar_bloque` para crear un buffer lleno de unos que se usa como patron de comparacion con `memcmp`.
- `memset(inodos, 0, sizeof(inodos))` en `initAI` para inicializar el bloque de inodos.
- `memset(inodoReservado.punterosDirectos, 0, sizeof(...))` en `reservar_inodo` para poner todos los punteros a cero.
- `memset(buffer, 0, TAMBUFFER)` en `mi_ls` para limpiar el buffer antes de listar.

**`memcpy`**: Se usa para copiar datos entre buffers:
- En `mi_write_f`: `memcpy(buf_bloque + desp1, buf_original, nbytes)` copia los datos del usuario al buffer de bloque en la posicion correcta.
- En `mi_read_f`: `memcpy(buf_original, buf_bloque + desp1, nbytes)` copia los datos leidos del bloque al buffer del usuario.

**`memcmp`**: Se usa en `reservar_bloque` para buscar rapidamente un bloque libre en el Mapa de Bits:
```c
if (memcmp(bufferMB, bufferAux, BLOCKSIZE) != 0) {
    // El bloque no esta lleno de 1s, tiene al menos un bit libre
    encontrado = 1;
}
```
En lugar de examinar byte a byte, `memcmp` compara todo el bloque de una vez contra un buffer lleno de `0xFF`. Si son iguales, el bloque del Mapa de Bits esta completamente ocupado y se puede saltar.

Tambien se usa en `liberar_indirectos_recursivo` para comprobar si un bloque de punteros quedo vacio tras la liberacion:
```c
memset(buffer_ceros, 0, BLOCKSIZE);
if (memcmp(bloque_punteros, buffer_ceros, BLOCKSIZE) == 0) {
    // El bloque de punteros esta vacio, se puede liberar
    liberar_bloque(*puntero_bloque);
}
```

**`strcpy`, `strlen`, `strcat`**: Se usan para manipulacion de cadenas en directorios y rutas:
- `strlen(camino)` para verificar si la ruta termina en `/`.
- `strcpy(buffer, "")` para inicializar buffers de cadena.
- `strcat(buffer, tmp)` para acumular lineas de salida en `mi_dir`.
- `strncpy(nueva_entrada.nombre, inicial, TAMNOMBRE)` para copiar nombres de entrada de directorio.

### 4.7. sem_open, sem_close, sem_unlink, sem_wait, sem_post

El proyecto usa semaforos POSIX con nombre para proteger las secciones criticas de asignacion y liberacion de bloques e inodos.

**`sem_open(SEM_NAME, O_CREAT, 0644, 1)`**: Crea o abre un semaforo con nombre `/sem_mi_sf`. El valor inicial `1` lo convierte en un mutex: solo un proceso puede entrar a la seccion critica a la vez. `O_CREAT` crea el semaforo si no existe. Los permisos `0644` permiten que cualquier proceso pueda abrirlo.

**`sem_wait`**: Decrementa el semaforo. Si el valor es 0, bloquea hasta que otro proceso haga `sem_post`. Se envuelve en `waitSem` y se llama como `mi_waitSem` desde las capas superiores.

**`sem_post`**: Incrementa el semaforo, permitiendo que otro proceso entre a la seccion critica. Se envuelve en `signalSem` y se llama como `mi_signalSem`.

**`sem_close`**: Cierra la referencia del proceso al semaforo. Se llama en `deleteSem` durante `bumount`.

El semaforo protege cuatro operaciones criticas:
1. **`reservar_bloque`**: Busqueda y marcaje de un bloque libre en el Mapa de Bits.
2. **`liberar_bloque`**: Desmarcaje de un bloque en el Mapa de Bits.
3. **`reservar_inodo`**: Obtencion del siguiente inodo libre de la lista enlazada.
4. **`liberar_inodo`**: Liberacion de un inodo y sus bloques asociados.

Sin el semaforo, dos procesos concurrentes podrian reservar el mismo bloque o el mismo inodo, corrompiendo la integridad del sistema de ficheros.

### 4.8. gettimeofday

```c
gettimeofday(&registro.fecha, NULL);
```

Se usa exclusivamente en `simulacion.c` para obtener marcas de tiempo con precision de microsegundos en los registros de escritura. A diferencia de `time(NULL)`, que solo da segundos, `gettimeofday` llena un `struct timeval` con campos `tv_sec` y `tv_usec`. Esto permite distinguir escrituras que ocurren en el mismo segundo pero en microsegundos diferentes.

El segundo argumento `NULL` indica que no nos interesa la zona horaria.

### 4.9. srand y rand

```c
srand(time(NULL) + getpid());
registro.nRegistro = rand() % REGMAX;
```

Cada hijo inicializa su generador de numeros aleatorios con `srand(time(NULL) + getpid())`. La combinacion de `time(NULL)` y `getpid()` es crucial: si solo se usara `time(NULL)`, dos hijos creados en el mismo segundo tendrian la misma semilla y generarian la misma secuencia de numeros aleatorios. Al sumar `getpid()`, cada hijo tiene una semilla unica porque cada proceso tiene un PID diferente.

`rand() % REGMAX` genera una posicion aleatoria entre 0 y 499999 (`REGMAX = 500000`) para escribir el registro en el fichero `prueba.dat`.

### 4.10. usleep

```c
usleep(50000);   // 50ms entre escrituras de un hijo
usleep(150000);  // 150ms entre forks del padre
```

`usleep` suspende la ejecucion del proceso durante un numero dado de microsegundos. Se usa en `simulacion.c` con dos propositos:

- **Entre escrituras de un hijo (50000 us = 50 ms)**: Simula un retardo realista entre operaciones de escritura, evitando que un hijo complete sus 50 escrituras instantaneamente y monopolice el semaforo.

- **Entre forks del padre (150000 us = 150 ms)**: Escalona la creacion de hijos, evitando que los 100 procesos se lancen simultaneamente. Esto reduce la contencion inicial en el semaforo y permite que el sistema maneje la concurrencia de forma mas gradual.

---

## 5. Gestion de errores

### 5.1. Codigos de retorno

El proyecto usa dos constantes globales para indicar el resultado de las operaciones:

- **`EXITO` (0)**: La operacion se completo correctamente.
- **`FALLO` (-1)**: Ocurrio un error durante la operacion.

Este esquema es consistente en todas las capas del sistema. Las funciones de nivel inferior (`bread`, `bwrite`, `leer_inodo`, `escribir_inodo`) retornan `EXITO` o `FALLO`. Las funciones de nivel superior (`mi_write_f`, `mi_read_f`, `mi_creat`, `mi_unlink`) propagan estos codigos o retornan valores especificos (como el numero de bytes escritos/leidos, o el numero de inodo).

### 5.2. Codigos de error de directorios

`directorios.h` define codigos de error especificos para operaciones de directorio, todos negativos para distinguirlos de `EXITO`:

```c
#define ERROR_CAMINO_INCORRECTO              -1  // Ruta mal formada
#define ERROR_PERMISO_LECTURA                -2  // Sin permiso de lectura
#define ERROR_NO_EXISTE_ENTRADA              -3  // Entrada no encontrada
#define ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO -4  // Directorio padre inexistente
#define ERROR_PERMISO_ESCRITURA              -5  // Sin permiso de escritura
#define ERROR_ENTRADA_YA_EXISTENTE           -6  // Entrada duplicada
#define ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_FICHERO -7  // Intento de crear dentro de un fichero
```

Estos codigos son retornados por `buscar_entrada` y propagados por las funciones de nivel superior (`mi_creat`, `mi_dir`, `mi_write`, `mi_read`, `mi_link`, `mi_unlink`).

### 5.3. Traduccion de errores a mensajes humanos

La funcion `mostrar_error_buscar_entrada` traduce los codigos de error a mensajes legibles:

```c
void mostrar_error_buscar_entrada(int error) {
    switch (error) {
    case ERROR_CAMINO_INCORRECTO:
        fprintf(stderr, "Error: Camino incorrecto.\n");
        break;
    case ERROR_PERMISO_LECTURA:
        fprintf(stderr, "Error: Permiso denegado de lectura..\n");
        break;
    case ERROR_NO_EXISTE_ENTRADA:
        fprintf(stderr, "Error: No existe el archivo o el directorio..\n");
        break;
    // ... etc
    }
}
```

Los comandos de usuario llaman a esta funcion cuando una operacion de directorio falla:

```c
int error = mi_creat(camino, permisos);
if (error < 0) {
    mostrar_error_buscar_entrada(error);
    bumount();
    return FALLO;
}
```

### 5.4. perror para errores de llamadas al sistema

En `bloques.c`, se usa `perror` para reportar errores de llamadas al sistema (`open`, `lseek`, `read`, `write`, `close`):

```c
if (descriptor == -1) {
    perror(RED "Error en bmount al abrir el fichero" RESET);
    return FALLO;
}
```

`perror` imprime el mensaje proporcionado seguido de la descripcion del error correspondiente al valor actual de `errno`. Por ejemplo, si `open` falla porque el fichero no existe y no se puede crear (directorio padre inexistente), `perror` imprimira algo como `Error en bmount al abrir el fichero: No such file or directory`.

Los colores ANSI (como `RED` y `RESET`) se usan para destacar los mensajes de error en la terminal.

### 5.5. Retornos directos y granularidad fina

En lugar de usar patrones antiguos como `goto _fin` para la limpieza de errores, el sistema utiliza retornos directos limpios asegurando la liberación del semáforo:

```c
    mi_waitSem();
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }
    mi_signalSem();
```

### 5.6. Liberacion de inodo en caso de error durante mi_write_f

En `buscar_entrada`, cuando se crea una nueva entrada de directorio, se reserva un inodo nuevo antes de escribir la entrada en el directorio padre:

```c
int ninodo_nuevo = reservar_inodo(tipo_inodo, permisos);
if (ninodo_nuevo == FALLO)
    return FALLO;

struct entrada nueva_entrada;
strncpy(nueva_entrada.nombre, inicial, TAMNOMBRE);
nueva_entrada.ninodo = ninodo_nuevo;

if (mi_write_f(*p_inodo_dir, &nueva_entrada, inodo_dir.tamEnBytesLog, sizeof(struct entrada)) == FALLO) {
    // Si falla la escritura, liberamos el inodo para no dejar basura
    liberar_inodo(ninodo_nuevo);
    return FALLO;
}
```

Si `mi_write_f` falla despues de reservar el inodo pero antes de escribir la entrada en el directorio, el inodo quedaria reservado pero inaccesible (no hay ninguna entrada de directorio que apunte a el). La llamada a `liberar_inodo(ninodo_nuevo)` evita esta fuga de recursos, devolviendo el inodo a la lista de inodos libres.

Este es un ejemplo de gestion de errores transaccional: si una operacion compuesta falla a mitad de camino, se deshacen los cambios parciales para mantener la consistencia del sistema de ficheros.
