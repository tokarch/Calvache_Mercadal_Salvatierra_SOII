# Documentacion de Arquitectura del Sistema de Ficheros

> Proyecto SOII - Sistema de ficheros basado en inodos sobre disco virtual
> Autores: Calvache Vidal, Antonio | Mercadal Forteza, Lluis | Salvatierra Romero, Diego Jeroni

---

## 1. Vision general de la arquitectura

Este proyecto implementa un sistema de ficheros completo en C, inspirado en el diseño de Unix, que opera sobre un fichero plano que simula un dispositivo de bloques. La arquitectura se organiza en capas estrictamente jerarquicas, donde cada nivel abstrae la complejidad del inferior y ofrece una interfaz mas rica al superior.

El disco virtual es un fichero ordinario de 100 MB. No se trata de un dispositivo real ni de un loop device montado por el kernel. Todo el manejo de bloques, inodos, directorios y concurrencia ocurre en espacio de usuario, usando las llamadas POSIX `read`, `write`, `lseek` sobre ese fichero. Esta decision de diseño permite depurar cada componente de forma aislada, sin depender del kernel ni de permisos de administrador.

El sistema se estructura en seis capas que van desde el acceso crudo a bloques hasta los comandos de usuario y la simulacion concurrente. Cada capa tiene una responsabilidad clara y depende exclusivamente de la capa inmediatamente inferior.

---

## 2. Jerarquia de modulos

```
    +----------------------------------------------------------+
    |  CAPA 6: Simulacion y verificacion                       |
    |  simulacion.c | verificacion.c                           |
    |  100 procesos fork() escribiendo concurrentemente        |
    +----------------------------------------------------------+
    |  CAPA 5: Comandos de usuario                             |
    |  mi_mkfs | mi_ls | mi_mkdir | mi_touch | mi_chmod       |
    |  mi_link | mi_rm | mi_rmdir | mi_escribir | mi_cat      |
    |  mi_stat                                                  |
    +----------------------------------------------------------+
    |  CAPA 4: Directorios                                     |
    |  directorios.c | directorios.h                            |
    |  extraer_camino | buscar_entrada | mi_creat | mi_dir    |
    |  mi_chmod | mi_stat | mi_write | mi_read                |
    |  mi_link | mi_unlink | cache UltimaEntrada               |
    +----------------------------------------------------------+
    |  CAPA 3: Operaciones de fichero                          |
    |  ficheros.c | ficheros.h                                 |
    |  mi_write_f | mi_read_f | mi_stat_f | mi_chmod_f        |
    |  mi_truncar_f                                            |
    +----------------------------------------------------------+
    |  CAPA 2: Gestion de metadatos                            |
    |  ficheros_basico.c | ficheros_basico.h                    |
    |  superbloque | bitmap | array de inodos                  |
    |  reservar_bloque | liberar_bloque                        |
    |  reservar_inodo | liberar_inodo                          |
    |  traducir_bloque_inodo (direccionamiento multinivel)     |
    +----------------------------------------------------------+
    |  CAPA 1: E/S de bloques                                  |
    |  bloques.c | bloques.h                                   |
    |  bmount | bumount | bread | bwrite                       |
    |  mi_waitSem | mi_signalSem (reentrada)                   |
    +----------------------------------------------------------+
    |  DISCO VIRTUAL (fichero plano, 100 MB)                   |
    |  Bloque 0: SB | MB | AI | Zona de datos                  |
    +----------------------------------------------------------+
```

**Por que esta jerarquia:** Cada capa resuelve un problema distinto. La capa 1 traduce operaciones de fichero POSIX en lecturas/escrituras de bloques de 1024 bytes. La capa 2 organiza esos bloques en estructuras de metadatos (superbloque, bitmap, inodos). La capa 3 ofrece operaciones de lectura/escritura a nivel de byte sobre un inodo. La capa 4 interpreta caminos tipo Unix y gestiona directorios. La capa 5 expone todo como programas independientes. La capa 6 estresa el sistema con concurrencia real.

---

## 3. Flujo de datos

El flujo de datos sigue siempre la misma direccion descendente. Un comando de usuario invoca una funcion de directorios, que a su vez delega en ficheros, que delega en ficheros_basico, que finalmente usa bloques para tocar el disco.

```
  Usuario
    |
    |  ./mi_escribir /directorio/fichero "hola mundo"
    v
  +-------------------+
  | mi_escribir.c     |  main(): valida args, bmount(), llama a mi_write(), bumount()
  +-------------------+
    |
    |  mi_write(camino, buf, offset, nbytes)
    v
  +-------------------+
  | directorios.c     |  buscar_entrada() resuelve el camino, obtiene ninodo
  |                   |  mi_write() delega en mi_write_f()
  +-------------------+
    |
    |  mi_write_f(ninodo, buf, offset, nbytes)
    v
  +-------------------+
  | ficheros.c        |  Calcula bloques logicos afectados
  |                   |  Fragmenta escritura en trozos de BLOCKSIZE
  |                   |  Llama a traducir_bloque_inodo() para cada bloque
  +-------------------+
    |
    |  traducir_bloque_inodo(ninodo, nblogico, reservar=1)
    v
  +-------------------+
  | ficheros_basico.c |  Navega arbol de punteros del inodo
  |                   |  12 directos + 3 niveles indirectos
  |                   |  Reserva bloques intermedios si no existen
  |                   |  Llama a reservar_bloque() -> bitmap
  +-------------------+
    |
    |  bread(nbloque, buf) / bwrite(nbloque, buf)
    v
  +-------------------+
  | bloques.c         |  lseek(fd, nbloque * BLOCKSIZE, SEEK_SET)
  |                   |  read() / write() de 1024 bytes
  +-------------------+
    |
    v
  [ DISCO VIRTUAL - fichero "disco" ]
```

Ninguna capa salta niveles. `directorios.c` nunca llama a `bread` directamente. `ficheros.c` nunca manipula el bitmap. Esta disciplina de capas es lo que permite razonar sobre el sistema de forma modular.

---

## 4. Flujo de ejecucion por comando

### 4.1 `mi_mkdir /a/b/c`

```
1. main() valida argumentos (necesita 1: el camino)
2. bmount("disco") -> abre fichero, inicializa semaforo
3. mi_creat("/a/b/c", permisos)
   |
   +-> buscar_entrada("/a/b/c", reservar=1)
       |
       +-> extraer_camino("/a/b/c") -> inicial="a", final="/b/c", tipo='d'
       +-> busca "a" en directorio raiz
       +-> Si existe y es directorio: recursar con "/b/c"
       +-> extraer_camino("/b/c") -> inicial="b", final="/c", tipo='d'
       +-> busca "b" en directorio "a"
       +-> Si existe y es directorio: recursar con "/c"
       +-> extraer_camino("/c") -> inicial="c", final="", tipo='f'
       +-> busca "c" en directorio "b"
       +-> No existe -> reservar=1:
           * reservar_inodo('d', permisos) -> nuevo inodo de directorio
           * reservar_bloque() -> bloque de datos vacio para las entradas
           * Crear entrada "." (apunta a si mismo) y ".." (apunta al padre)
           * Anadir entrada "c" al directorio padre "b"
       +-> Devuelve ninodo del nuevo directorio
4. bumount() -> cierra fichero, elimina semaforo
```

### 4.2 `mi_escribir /fichero "contenido" offset`

```
1. main() valida argumentos (camino, texto, offset)
2. bmount("disco")
3. mi_write("/fichero", buf, offset, strlen(buf))
   |
   +-> buscar_entrada("/fichero", reservar=0) -> obtiene ninodo
   +-> Verifica que es fichero (tipo == 'f')
   +-> mi_write_f(ninodo, buf, offset, nbytes)
       |
       +-> leer_inodo(ninodo) -> verifica permisos de escritura
       +-> Calcula primerBL = offset / BLOCKSIZE
       +-> Calcula ultimoBL = (offset + nbytes - 1) / BLOCKSIZE
       +-> Bucle sobre cada bloque logico afectado:
           * traducir_bloque_inodo(ninodo, nblogico, reservar=1)
             -> obtiene nbloque fisico (reserva si no existe)
           * bread(nbloque, buf_bloque) -> lee bloque existente
           * memcpy(buf_bloque + desp, buf_original + offset, nbytes)
           * bwrite(nbloque, buf_bloque) -> escribe bloque modificado
       +-> Actualiza inodo.tamEnBytesLog si la escritura extendio el fichero
       +-> escribir_inodo(ninodo) -> persiste metadatos
4. bumount()
```

### 4.3 `mi_cat /fichero`

```
1. main() valida argumentos
2. bmount("disco")
3. mi_read("/fichero", buf, offset=0, nbytes=tamano)
   |
   +-> buscar_entrada("/fichero", reservar=0) -> obtiene ninodo
   +-> mi_read_f(ninodo, buf, offset, nbytes)
       |
       +-> leer_inodo(ninodo) -> verifica permisos de lectura
       +-> Calcula bloques logicos afectados
       +-> Bucle sobre cada bloque logico:
           * traducir_bloque_inodo(ninodo, nblogico, reservar=0)
             -> si puntero es 0, bloque no asignado (hueco/sparse)
           * Si bloque existe: bread(nbloque, buf_bloque)
           * Si bloque no existe: memset(buf_bloque, 0) (hueco)
           * memcpy(buf_original, buf_bloque + desp, nbytes)
       +-> Respeta EOF: no lee mas alla de tamEnBytesLog
4. printf("%s", buf) -> muestra contenido en stdout
5. bumount()
```

### 4.4 `mi_mkfs disco 100000`

```
1. main() valida argumentos (nombre, nbloques)
2. open("disco", O_RDWR|O_CREAT|O_TRUNC) -> crea/trunca fichero
3. ftruncate() -> asigna nbloques * BLOCKSIZE bytes
4. bmount("disco")
5. initSB(nbloques, ninodos) -> escribe superbloque en bloque 0
6. initMB() -> inicializa bitmap a todo 0 (libre)
7. initAI() -> inicializa array de inodos como lista enlazada LIFO
8. reservar_inodo('d', 6) -> crea inodo raiz (permisos rwxrwx---)
9. reservar_bloque() -> asigna primer bloque de datos para raiz
10. Crear entradas "." y ".." en el bloque de raiz
11. bumount()
```

---

## 5. Interaccion usuario -> shell -> gestor

```
  Terminal (bash)
    |
    |  $ ./mi_ls /directorio
    v
  +------------------+
  | Shell (bash)     |  fork() + execve("./mi_ls")
  |                  |  El shell no sabe nada del SF interno
  +------------------+
    |
    v
  +------------------+
  | mi_ls.c (main)   |  1. bmount("disco")
  |                  |  2. mi_dir(camino, buffer, tipo)
  |                  |  3. printf(buffer) -> muestra entradas
  |                  |  4. bumount()
  +------------------+
    |
    |  Llama a funciones del SF como libreria
    v
  +------------------+
  | directorios.c    |  mi_dir() -> buscar_entrada() -> lee entradas
  |                  |  del bloque de datos del directorio
  +------------------+
    |
    v
  [ Disco virtual ]

  Cada comando es un proceso independiente:
  - No hay demonio ni servidor persistente
  - Cada ejecucion monta y desmonta el disco
  - El semaforo POSIX "/sem_mi_sf" coordina el acceso entre procesos
```

El modelo es deliberadamente simple. No existe un proceso servidor del sistema de ficheros. Cada comando es un programa completo que monta el disco, realiza su operacion y lo desmonta. La concurrencia se gestiona mediante un semaforo con nombre compartido entre todos los procesos, no mediante un coordinador central.

---

## 6. Manejo de procesos (simulacion.c)

La simulacion crea 100 procesos hijos que escriben concurrentemente en el mismo sistema de ficheros. Es la prueba de fuego para la sincronizacion.

```
  Proceso padre (simulacion)
    |
    |  bmount(argv[1])
    |  mi_creat("/simul_aaaammddhhmmss/")
    |  signal(SIGCHLD, reaper)
    |
    +-- fork() --> Hijo 1 (PID X)
    |              bmount(argv[1])  <- descriptor independiente
    |              mi_creat("/simul_.../proceso_X/")
    |              mi_creat("/simul_.../proceso_X/prueba.dat")
    |              Bucle 50 escrituras:
    |                mi_write() con registros aleatorios
    |                Posicion aleatoria 0..REGMAX
    |              bumount()
    |              exit(0)
    |
    +-- fork() --> Hijo 2 (PID Y)
    |              ... mismo patron ...
    |
    +-- fork() --> Hijo 3 (PID Z)
    |              ... mismo patron ...
    |
    +-- ... hasta 100 hijos ...
    |
    |  wait() hasta que acabados == NUMPROCESOS
    |  bumount()
    v

  Senal SIGCHLD:
    - El padre registra reaper() como manejador de SIGCHLD
    - Cada vez que un hijo termina, el kernel envia SIGCHLD
    - reaper() llama waitpid(-1, NULL, WNOHANG) en bucle
    - waitpid con WNOHANG reap zombies sin bloquear
    - Incrementa contador 'acabados'
    - El padre espera en bucle while(acabados < NUMPROCESOS)
      con sleep(1) entre comprobaciones
```

**Detalle critico del fork:** Cuando el padre hace `fork()`, el hijo hereda el descriptor de fichero del padre. Ambos compartirian el mismo puntero de posicion (`lseek`), lo que causaria lecturas/escrituras corruptas. Por eso cada hijo llama a `bmount()` de nuevo, que detecta `descriptor > 0` y lo cierra antes de abrir uno nuevo. Asi cada proceso tiene su propio descriptor independiente.

---

## 7. Manejo de directorios y ficheros

### 7.1 Estructura del inodo

Cada inodo ocupa exactamente 128 bytes y contiene toda la metadata de un fichero o directorio:

```
  struct inodo (128 bytes)
  +---------------------------+
  | tipo: 'l'/'d'/'f'  (1 B)  |  <- libre, directorio, fichero
  | permisos: rwx      (1 B)  |  <- bits: 4=r, 2=w, 1=x
  | reservado_alineacion (6B) |  <- padding para alineacion 64-bit
  +---------------------------+
  | atime              (8 B)  |  <- ultimo acceso
  | mtime              (8 B)  |  <- ultima modificacion de datos
  | ctime              (8 B)  |  <- ultima modificacion de metadata
  | btime              (8 B)  |  <- creacion (birth)
  +---------------------------+
  | nlinks             (4 B)  |  <- numero de enlaces duros
  | tamEnBytesLog      (4 B)  |  <- tamaño logico (EOF)
  | numBloquesOcupados (4 B)  |  <- bloques fisicos asignados
  +---------------------------+
  | punterosDirectos[12]      |  <- 12 x 4 = 48 bytes
  |   [0] ... [11]            |     bloques 0..11 directos
  +---------------------------+
  | punterosIndirectos[3]     |  <- 3 x 4 = 12 bytes
  |   [0] -> indirecto simple |     256 punteros -> bloques 12..267
  |   [1] -> doble indirecto  |     256^2 -> bloques 268..65803
  |   [2] -> triple indirecto |     256^3 -> hasta 16.8M bloques
  +---------------------------+
  | padding            (16 B) |  <- relleno hasta 128 bytes exactos
  +---------------------------+
```

**Direccionamiento multinivel:** Con 12 punteros directos y 3 niveles indirectos de 256 punteros cada bloque, un fichero puede alcanzar:

| Nivel | Rango de bloques | Capacidad maxima |
|-------|-----------------|------------------|
| Directos | 0 - 11 | 12 KB |
| Indirecto simple | 12 - 267 | 268 KB |
| Doble indirecto | 268 - 65.803 | ~64 MB |
| Triple indirecto | 65.804 - 16.843.020 | ~16 GB |

### 7.2 Arbol de directorios

Los directorios son ficheros especiales cuyo contenido es una secuencia de entradas de 64 bytes:

```
  struct entrada (64 bytes)
  +-------------------------------------------+
  | nombre[60]  (60 bytes)                    |  <- nombre del fichero/dir
  | ninodo      (4 bytes)                     |  <- numero de inodo
  +-------------------------------------------+

  Bloque de directorio (1024 bytes):
  +----------+----------+----------+ ... +----------+
  | entrada0 | entrada1 | entrada2 |     | entrada15|
  +----------+----------+----------+ ... +----------+
  16 entradas por bloque (1024 / 64 = 16)
```

Cada directorio contiene al menos dos entradas especiales:
- `.` -> apunta al propio inodo del directorio
- `..` -> apunta al inodo del directorio padre

### 7.3 Resolucion de caminos (buscar_entrada)

La funcion `buscar_entrada` es recursiva y resuelve caminos completos:

```
  buscar_entrada("/a/b/c", reservar=1)
    |
    +-> extraer_camino("/a/b/c") -> inicial="a", final="/b/c", tipo='d'
    +-> Busca "a" en el directorio raiz (inodo raiz)
    |
    +-> Si "a" existe y es directorio:
    |     buscar_entrada("/b/c", inodo_de_a, reservar=1)
    |       |
    |       +-> extraer_camino("/b/c") -> inicial="b", final="/c", tipo='d'
    |       +-> Busca "b" en directorio "a"
    |       +-> Si "b" existe y es directorio:
    |             buscar_entrada("/c", inodo_de_b, reservar=1)
    |               |
    |               +-> extraer_camino("/c") -> inicial="c", final="", tipo='f'
    |               +-> Busca "c" en directorio "b"
    |               +-> Si no existe y reservar=1:
    |                     * reservar_inodo('f', permisos)
    |                     * Anadir entrada "c" al bloque de "b"
    |                     * Si bloque lleno, reservar otro bloque
    |               +-> Devuelve ninodo de "c"
    |
    +-> Si "a" NO existe y reservar=1:
          ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO
```

La recursion termina cuando `final` esta vacio, es decir, cuando se ha llegado al ultimo componente del camino.

---

## 8. Gestion de memoria y caches

### 8.1 Cache UltimaEntrada

Para evitar resolver el mismo camino una y otra vez, `directorios.c` mantiene dos caches globales:

```
  struct UltimaEntrada {
      char camino[TAMNOMBRE * PROFUNDIDAD];  // 60 * 32 = 1920 bytes
      int p_inodo;                            // inodo resuelto
  };

  static struct UltimaEntrada UltimaEntradaLectura;   // cache para mi_read
  static struct UltimaEntrada UltimaEntradaEscritura; // cache para mi_write
```

**Como funciona:**

```
  mi_write("/a/b/c/fichero", buf, offset, nbytes)
    |
    +-> strcmp(camino, UltimaEntradaEscritura.camino) == 0?
    |     SI -> usar UltimaEntradaEscritura.p_inodo directamente
    |     NO  -> buscar_entrada(camino) -> actualizar cache
    |
    +-> mi_write_f(ninodo_cacheado, buf, offset, nbytes)

  mi_read("/a/b/c/fichero", buf, offset, nbytes)
    |
    +-> strcmp(camino, UltimaEntradaLectura.camino) == 0?
    |     SI -> usar UltimaEntradaLectura.p_inodo directamente
    |     NO  -> buscar_entrada(camino) -> actualizar cache
    |
    +-> mi_read_f(ninodo_cacheado, buf, offset, nbytes)
```

La cache se separa en lectura y escritura porque un fichero puede abrirse simultaneamente para ambos propositos, y el ultimo camino de lectura no tiene por que coincidir con el ultimo de escritura.

**Limitacion:** La cache solo almaciona la ultima entrada. No es una tabla hash ni un LRU. Si se accede a un tercer fichero distinto, se pierde la referencia al primero. Es una optimizacion sencilla pero efectiva para el patron tipico de acceso secuencial.

### 8.2 Layout del disco virtual

El disco se organiza en cuatro zonas consecutivas:

```
  Bloque 0          Bloque 1..N        Bloque N+1..M      Bloque M+1..fin
  +----------------+------------------+------------------+----------------+
  | SUPERBLOQUE    | MAPA DE BITS     | ARRAY DE INODOS  | ZONA DE DATOS  |
  | (1 bloque)     | (variable)       | (variable)       | (resto)        |
  | posSB = 0      | posPrimerBloqueMB| posPrimerBloqueAI| posPrimerBloque|
  |                | posUltimoBloqueMB| posUltimoBloqueAI| posUltimoBloque|
  +----------------+------------------+------------------+----------------+

  Superbloque (bloque 0):
  - Metadatos globales del sistema de ficheros
  - Posiciones absolutas de cada zona
  - Contadores de bloques/inodos libres y totales
  - Posicion del inodo raiz

  Mapa de bits:
  - 1 bit por bloque de la zona de datos
  - Bit 0 = bloque libre, bit 1 = bloque ocupado
  - Tamaño: ceil(tamBloquesDatos / 8) bloques

  Array de inodos:
  - Inodos de 128 bytes, 8 por bloque
  - Organizados como lista enlazada LIFO
  - Cada inodo libre apunta al siguiente libre
  - posPrimerInodoLibre en el superbloque es la cabeza

  Zona de datos:
  - Bloques asignados dinamicamente via bitmap
  - Contienen contenido de ficheros y entradas de directorio
  - Bloques intermedios de indirecciones tambien viven aqui
```

---

## 9. Sincronizacion y concurrencia

### 9.1 Semaforo POSIX con nombre

El sistema usa un semaforo POSIX con nombre `"/sem_mi_sf"` como mutex global:

```
  semaforo_mutex_posix.c
    |
    +-> initSem()
    |     sem_open("/sem_mi_sf", O_CREAT, 0644, 1)
    |     Valor inicial = 1 -> comportamiento de mutex
    |     O_CREAT -> lo crea si no existe
    |     Compartido entre procesos (named semaphore)
    |
    +-> waitSem() -> sem_wait() -> decrementa, bloquea si vale 0
    +-> signalSem() -> sem_post() -> incrementa, despierta un bloqueado
    +-> deleteSem() -> sem_close() + sem_unlink()
```

### 9.2 Secciones criticas

Las operaciones que modifican estado compartido estan protegidas:

```
  reservar_bloque():
    mi_waitSem()
      leer_bit() -> busca bloque libre en bitmap
      escribir_bit() -> marca como ocupado
      SB.cantBloquesLibres--
      escribir_inodo(SB) -> persiste superbloque
    mi_signalSem()

  liberar_bloque():
    mi_waitSem()
      escribir_bit(nbloque, 0) -> libera en bitmap
      SB.cantBloquesLibres++
      escribir_inodo(SB)
    mi_signalSem()

  reservar_inodo():
    mi_waitSem()
      leer_inodo(posPrimerInodoLibre)
      inodo.tipo = tipo
      inodo.permisos = permisos
      posPrimerInodoLibre = siguiente libre
      SB.cantInodosLibres--
      escribir_inodo()
    mi_signalSem()

  liberar_inodo():
    mi_waitSem()
      inodo.tipo = 'l'
      inodo.siguienteInodo = posPrimerInodoLibre
      posPrimerInodoLibre = ninodo
      SB.cantInodosLibres++
      escribir_inodo()
    mi_signalSem()

  traducir_bloque_inodo():
    mi_waitSem()
      navega arbol de punteros
      si puntero == 0 y reservar: reservar_bloque()
      actualiza puntero en inodo o bloque indirecto
    mi_signalSem()

  mi_write_f():
    mi_waitSem()
      leer_inodo()
      bucle sobre bloques logicos:
        traducir_bloque_inodo()
        bread() / bwrite()
      actualizar tamEnBytesLog
      escribir_inodo()
    mi_signalSem()
```

### 9.3 Contador de reentrada

`bloques.c` mantiene un contador `inside_sc` para permitir reentrada:

```
  static unsigned int inside_sc = 0;

  mi_waitSem():
    if (inside_sc == 0) {
        sem_wait(mutex);  // Primera entrada: adquiere semaforo
    }
    inside_sc++;          // Incrementa contador de reentrada

  mi_signalSem():
    inside_sc--;          // Decrementa contador
    if (inside_sc == 0) {
        sem_post(mutex);  // Ultima salida: libera semaforo
    }
```

Esto permite que `mi_write_f` llame a `traducir_bloque_inodo`, que a su vez llama a `reservar_bloque`, sin deadlock. Todas estas funciones llaman a `mi_waitSem`, pero como el contador ya es mayor que 0, no intentan adquirir el semaforo de nuevo.

### 9.4 Modelo de procesos de la simulacion

```
  Tiempo
    |
    v

  Padre:  [bmount]---[crear dir simul]---[fork x100]---[wait loop]---[bumount]
            |
  Hijo 1:   +----[bmount]---[crear dir]---[50 writes]---[bumount]---[exit]
  Hijo 2:   +----[bmount]---[crear dir]---[50 writes]---[bumount]---[exit]
  Hijo 3:   +----[bmount]---[crear dir]---[50 writes]---[bumount]---[exit]
  ...
  Hijo 100: +----[bmount]---[crear dir]---[50 writes]---[bumount]---[exit]

  Cada escritura de cada hijo:
    mi_write() -> buscar_entrada() -> mi_write_f()
      -> mi_waitSem() [adquiere semaforo]
      -> traducir_bloque_inodo() -> reservar_bloque() [secciones criticas]
      -> bread/bwrite [I/O al disco]
      -> escribir_inodo [actualiza metadata]
      -> mi_signalSem() [libera semaforo]

  El semaforo serializa el acceso al disco:
  - Solo un proceso escribe a la vez
  - Los demas bloquean en sem_wait()
  - No hay race conditions en bitmap ni inodos
```

---

## 10. Diagramas ASCII

### 10.1 Jerarquia de capas (vista completa)

```
  +================================================================+
  |                    SISTEMA DE FICHEROS SOII                    |
  +================================================================+
  |                                                                |
  |  +----------------------------------------------------------+  |
  |  | CAPA 6: SIMULACION                                       |  |
  |  |                                                          |  |
  |  |  simulacion.c          verificacion.c                    |  |
  |  |  - fork() x 100        - lee estructura simulacion       |  |
  |  |  - 50 writes/hijo      - verifica coherencia registros   |  |
  |  |  - SIGCHLD/reaper      - genera informe.txt              |  |
  |  +----------------------------------------------------------+  |
  |                            ^                                   |
  |  +----------------------------------------------------------+  |
  |  | CAPA 5: COMANDOS DE USUARIO                              |  |
  |  |                                                          |  |
  |  |  mi_mkfs  mi_ls  mi_mkdir  mi_touch  mi_chmod            |  |
  |  |  mi_link  mi_rm  mi_rmdir  mi_escribir  mi_cat  mi_stat  |  |
  |  |                                                          |  |
  |  |  Patron comun: main -> bmount -> funcion SF -> bumount   |  |
  |  +----------------------------------------------------------+  |
  |                            ^                                   |
  |  +----------------------------------------------------------+  |
  |  | CAPA 4: DIRECTORIOS                                      |  |
  |  |                                                          |  |
  |  |  extraer_camino()     -> parsea componentes del camino   |  |
  |  |  buscar_entrada()     -> resolucion recursiva de paths   |  |
  |  |  mi_creat()           -> crea fichero o directorio       |  |
  |  |  mi_dir()             -> lista contenido de directorio   |  |
  |  |  mi_write()/mi_read() -> I/O por camino con cache        |  |
  |  |  mi_link()/mi_unlink()-> enlaces duros y borrado         |  |
  |  |  UltimaEntrada        -> cache de ultimo camino          |  |
  |  +----------------------------------------------------------+  |
  |                            ^                                   |
  |  +----------------------------------------------------------+  |
  |  | CAPA 3: OPERACIONES DE FICHERO                           |  |
  |  |                                                          |  |
  |  |  mi_write_f()  -> escribe bytes fragmentados en bloques  |  |
  |  |  mi_read_f()   -> lee bytes respetando EOF y huecos      |  |
  |  |  mi_stat_f()   -> devuelve struct STAT                   |  |
  |  |  mi_chmod_f()  -> cambia permisos                        |  |
  |  |  mi_truncar_f() -> libera bloques desde offset           |  |
  |  +----------------------------------------------------------+  |
  |                            ^                                   |
  |  +----------------------------------------------------------+  |
  |  | CAPA 2: METADATOS                                        |  |
  |  |                                                          |  |
  |  |  superbloque   -> bloque 0, metadatos globales           |  |
  |  |  bitmap        -> 1 bit/bloque, alloc/free               |  |
  |  |  array inodos  -> lista LIFO, 128 bytes cada uno         |  |
  |  |  traducir_bloque_inodo() -> arbol de punteros multinivel |  |
  |  |  reservar/liberar_bloque() -> gestion bitmap             |  |
  |  |  reservar/liberar_inodo()  -> gestion lista inodos       |  |
  |  +----------------------------------------------------------+  |
  |                            ^                                   |
  |  +----------------------------------------------------------+  |
  |  | CAPA 1: E/S DE BLOQUES                                   |  |
  |  |                                                          |  |
  |  |  bmount()   -> open() + initSem()                        |  |
  |  |  bumount()  -> close() + deleteSem()                     |  |
  |  |  bread()    -> lseek() + read()  (1024 bytes)            |  |
  |  |  bwrite()   -> lseek() + write() (1024 bytes)            |  |
  |  |  mi_waitSem/SignalSem -> reentrada con contador          |  |
  |  +----------------------------------------------------------+  |
  |                            ^                                   |
  |  +----------------------------------------------------------+  |
  |  | DISCO VIRTUAL                                              |  |
  |  |  Fichero plano de 100 MB                                   |  |
  |  |  [SB][MB...][AI...][Datos.............................]    |  |
  |  +----------------------------------------------------------+  |
  |                                                                |
  +================================================================+
```

### 10.2 Flujo de datos para lectura y escritura

```
  ESCRITURA: mi_write_f(ninodo, "hola mundo", offset=500, nbytes=10)
  ===================================================================

  inodo.tamEnBytesLog = 1000
  primerBL = 500 / 1024 = 0
  ultimoBL = (500 + 10 - 1) / 1024 = 0

  Solo afecta al bloque logico 0:

    +---------+
    | inodo   |  punterosDirectos[0] = 5000 (bloque fisico)
    +---------+
         |
         v
    +---------+  bread(5000, buf_bloque)
    | bloque  |  memcpy(buf_bloque + 500, "hola mundo", 10)
    | fisico  |  bwrite(5000, buf_bloque)
    | 5000    |
    +---------+
    offset:  0.........500........1023
    datos:   [xxxxxxxxxx|hola mundo|xxxxxxxx]

  inodo.tamEnBytesLog se actualiza si offset + nbytes > tam actual


  LECTURA: mi_read_f(ninodo, buf, offset=2700, nbytes=200)
  =========================================================

  inodo.tamEnBytesLog = 5000
  primerBL = 2700 / 1024 = 2
  ultimoBL = (2700 + 200 - 1) / 1024 = 2

  Bloque logico 2 -> dentro de rango indirecto simple:

    +---------+
    | inodo   |  punterosIndirectos[0] = 3000 (bloque indirecto)
    +---------+
         |
         v
    +---------+  bread(3000, buf_indirecto)
    | bloque  |  buf_indirecto[268-12] = 7000 (bloque fisico)
    | indirect|
    | 3000    |
    +---------+
         |
         v
    +---------+  bread(7000, buf_bloque)
    | bloque  |  memcpy(buf, buf_bloque + 652, 200)
    | fisico  |  (desp = 2700 % 1024 = 652)
    | 7000    |
    +---------+
```

### 10.3 Resolucion de caminos

```
  buscar_entrada("/home/user/docs/fichero.txt", reservar=0)
  ==========================================================

  Nivel 1: extraer_camino("/home/user/docs/fichero.txt")
    inicial = "home", final = "/user/docs/fichero.txt", tipo = 'd'
    Buscar "home" en raiz (/) -> encontrado, inodo 5, es directorio

  Nivel 2: extraer_camino("/user/docs/fichero.txt")
    inicial = "user", final = "/docs/fichero.txt", tipo = 'd'
    Buscar "user" en inodo 5 -> encontrado, inodo 12, es directorio

  Nivel 3: extraer_camino("/docs/fichero.txt")
    inicial = "docs", final = "/fichero.txt", tipo = 'd'
    Buscar "docs" en inodo 12 -> encontrado, inodo 23, es directorio

  Nivel 4: extraer_camino("/fichero.txt")
    inicial = "fichero.txt", final = "", tipo = 'f'
    Buscar "fichero.txt" en inodo 23 -> encontrado, inodo 45

  Resultado: p_inodo = 45, p_entrada = indice en bloque de inodo 23

  Arbol resultante:

    / (inodo 1, raiz)
    |
    +-- home (inodo 5, dir)
        |
        +-- user (inodo 12, dir)
            |
            +-- docs (inodo 23, dir)
            |   |
            |   +-- fichero.txt (inodo 45, fichero)
            |
            +-- .  -> inodo 12
            +-- .. -> inodo 5
```

### 10.4 Modelo de procesos de la simulacion

```
  +----------------------------------------------------------------+
  |                     PROCESO PADRE                              |
  |                                                                |
  |  bmount("disco")                                               |
  |  mi_creat("/simul_20250507143022/")                            |
  |  signal(SIGCHLD, reaper)                                       |
  |                                                                |
  |  for (proceso = 1; proceso <= 100; proceso++)                  |
  |    fork()                                                      |
  |      |                                                         |
  |      +-- Hijo: bmount("disco") <- descriptor nuevo             |
  |      |       mi_creat("/simul_.../proceso_PID/")               |
  |      |       mi_creat("/simul_.../proceso_PID/prueba.dat")     |
  |      |       for (escritura = 1; escritura <= 50; escritura++) |
  |      |         nRegistro = rand() % REGMAX                     |
  |      |         offset = nRegistro * sizeof(REGISTRO)           |
  |      |         mi_write(camino, &registro, offset, sizeof())   |
  |      |       bumount()                                         |
  |      |       exit(0)                                           |
  |                                                                |
  |  while (acabados < 100)                                        |
  |    sleep(1)  <- espera activa con reap de zombies              |
  |                                                                |
  |  bumount()                                                     |
  +----------------------------------------------------------------+

  Sincronizacion entre hijos:

  Hijo A:  [mi_waitSem] -> [escribe] -> [mi_signalSem]
  Hijo B:  [bloqueado en sem_wait] -----> [adquiere] -> [escribe] -> [libera]
  Hijo C:  [bloqueado en sem_wait] -----------------------> [adquiere] -> ...

  El semaforo "/sem_mi_sf" garantiza exclusion mutua:
  - Solo un hijo accede al disco a la vez
  - bitmap, inodos y superbloque nunca se corrompen
  - Los registros se escriben atomicamente (cada write es < BLOCKSIZE)
```

---

## Resumen de dependencias entre modulos

```
  simulacion.c ─────────────────┐
  verificacion.c ───────────────┤
  mi_mkfs.c ────────────────────┤
  mi_ls.c ──────────────────────┤
  mi_mkdir.c ───────────────────┤
  mi_touch.c ───────────────────┤     directorios.h
  mi_chmod.c ───────────────────┤         │
  mi_link.c ────────────────────┤         │
  mi_rm.c ──────────────────────┤         ▼
  mi_rmdir.c ───────────────────┼──> directorios.c ──> ficheros.h
  mi_escribir.c ────────────────┤                           │
  mi_cat.c ─────────────────────┤                           ▼
  mi_stat.c ────────────────────┤                     ficheros.c ──> ficheros_basico.h
  leer_sf.c ────────────────────┤                                           │
                                │                                           ▼
                          ficheros_basico.c ──> bloques.h
                                                    │
                                                    ▼
                                              bloques.c ──> semaforo_mutex_posix.h
                                                              │
                                                              ▼
                                                        semaforo_mutex_posix.c
```

Cada modulo solo importa la cabecera de la capa inmediatamente inferior. Esta disciplina de dependencias es lo que permite compilar por niveles y probar cada capa de forma independiente.
