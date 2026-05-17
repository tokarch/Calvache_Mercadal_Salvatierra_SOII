# Sistema de Ficheros basado en Inodos

Implementacion en C de un sistema de ficheros sobre un dispositivo virtual, desarrollado como proyecto docente para la asignatura Sistemas Operativos II (SOII).

**Autores**: Calvache Vidal Antonio, Mercadal Forteza Lluis, Salvatierra Romero Diego Jeroni

---

## Descripcion del proyecto

Este proyecto implementa un sistema de ficheros inspirado en Unix, donde los metadatos y los datos se almacenan en un fichero que actua como disco virtual. El diseno sigue el modelo clasico de inodos: cada fichero o directorio tiene asociado un inodo que contiene su informacion de control (tipo, permisos, timestamps, punteros a bloques), mientras que el contenido real se guarda en bloques de datos independientes.

El disco virtual se estructura en cuatro zonas diferenciadas: Superbloque, Mapa de Bits, Array de Inodos y zona de Datos. Todas las operaciones de lectura y escritura pasan por una capa de abstraccion de bloques (`bread`/`bwrite`) que traduce las operaciones logicas a llamadas al sistema `pread`/`pwrite` sobre el fichero de disco.

## Objetivo del gestor de ficheros

Construir un sistema de ficheros funcional que permita:

- Crear y formatear un dispositivo virtual con estructura de bloques, inodos y directorios.
- Gestionar la asignacion y liberacion de bloques de datos e inodos mediante mapa de bits y lista enlazada de inodos libres.
- Resolver rutas absolutas completas con navegacion por directorios anidados.
- Soportar operaciones de lectura, escritura, truncado, cambio de permisos y estadisticas de ficheros.
- Crear enlaces duros entre ficheros y eliminar entradas de directorio con liberacion recursiva de bloques.
- Garantizar la consistencia del sistema ante acceso concurrente mediante semaforos POSIX.
- Simular y verificar el comportamiento bajo concurrencia real con 100 procesos escribiendo simultaneamente.

## Caracteristicas principales

### Direccionamiento de bloques

Cada inodo dispone de 12 punteros directos y 3 punteros indirectos (simple, doble y triple). Con un tamano de bloque de 1024 bytes y punteros de 4 bytes, cada bloque de indirecciones almacena 256 punteros:

| Nivel | Punteros | Bloques logicos maximos acumulados |
|-------|----------|-----------------------------------|
| Directos | 12 | 12 |
| Indirecto simple | 256 | 268 |
| Doble indirecto | 256^2 = 65.536 | 65.804 |
| Triple indirecto | 256^3 = 16.777.216 | 16.843.020 |

Esto permite que un solo fichero pueda alcanzar aproximadamente 16,8 millones de bloques, lo que equivale a unos 17 GB con bloques de 1024 bytes.

### Entradas de directorio

Cada entrada ocupa 64 bytes exactos: 60 bytes para el nombre del fichero y 4 bytes para el numero de inodo asociado. En un bloque de 1024 bytes caben 16 entradas. La profundidad maxima del arbol de directorios se limita a 32 niveles.

### Lista de inodos libres

Los inodos liberados no se destruyen, se reinsertan en una lista enlazada. El campo `punterosDirectos[0]` del inodo libre almacena el numero del siguiente inodo libre. El superbloque mantiene la referencia al primer inodo libre disponible, lo que permite reservar y liberar inodos en tiempo constante.

### Semaforos POSIX (Nivel 11+)

Las secciones criticas que acceden al mapa de bits, al array de inodos o a la zona de datos se protegen con un semaforo mutex POSIX nombrado (`/my_semaphore`). Las funciones `mi_waitSem()` y `mi_signalSem()` envuelven las llamadas `wait()` y `signal()` del semaforo, garantizando exclusion mutua entre procesos concurrentes.

### Cache de directorio (Nivel 9)

Para optimizar las operaciones de lectura y escritura sobre ficheros, se mantiene en memoria la ultima entrada de directorio leida o escrita. Si la siguiente operacion consulta el mismo camino, se evita la busqueda completa en el arbol de directorios.

### Sistema de depuracion

Las macros `DEBUGN3` a `DEBUGN8` controlan trazas `fprintf` condicionales compiladas en tiempo de preprocesador. Cada nivel activa sus propias trazas, lo que permite inspeccionar el comportamiento interno sin penalizar el rendimiento en compilaciones de produccion.

## Arquitectura general

El sistema se organiza en capas, donde cada nivel construye sobre el anterior:

```
┌─────────────────────────────────────────────────┐
│  Comandos de usuario (mi_*, simulacion, etc.)   │  Nivel 7-13
├─────────────────────────────────────────────────┤
│  Capa de directorios (mi_creat, mi_link, ...)   │  Nivel 7-10
├─────────────────────────────────────────────────┤
│  Capa de ficheros (mi_write_f, mi_read_f, ...)  │  Nivel 5-6
├─────────────────────────────────────────────────┤
│  Traduccion de bloques (traducir_bloque_inodo)  │  Nivel 4
├─────────────────────────────────────────────────┤
│  Inodos y mapa de bits (reservar, liberar)      │  Nivel 2-3
├─────────────────────────────────────────────────┤
│  Superbloque (initSB, lectura de metadatos)     │  Nivel 2
├─────────────────────────────────────────────────┤
│  E/S de bloques (bread, bwrite, bmount)         │  Nivel 1
├─────────────────────────────────────────────────┤
│  Semaforo mutex POSIX (waitSem, signalSem)      │  Nivel 11
├─────────────────────────────────────────────────┤
│  Fichero de disco (disco)                       │
└─────────────────────────────────────────────────┘
```

La disposicion fisica del disco sigue este orden:

```
Bloque 0          → Superbloque (1 bloque)
Bloques 1..N      → Mapa de Bits (1 bit por bloque de datos)
Bloques N+1..M    → Array de Inodos (inodos de 128 bytes)
Bloques M+1..fin  → Bloques de Datos
```

El superbloque almacena las posiciones absolutas de inicio y fin de cada zona, junto con contadores de bloques e inodos libres y totales.

## Componentes principales

### Capa de E/S de bloques (`bloques.c/h`)

Gestiona el montaje y desmontaje del disco virtual y proporciona las primitivas de lectura y escritura de bloques completos.

| Funcion | Descripcion |
|---------|-------------|
| `bmount()` | Abre el fichero de disco y lo asocia al descriptor global |
| `bumount()` | Cierra el fichero de disco |
| `bread()` | Lee un bloque completo (1024 bytes) por su numero absoluto |
| `bwrite()` | Escribe un bloque completo por su numero absoluto |
| `mi_waitSem()` | Adquiere el semaforo mutex |
| `mi_signalSem()` | Libera el semaforo mutex |

### Metadatos y gestion basica (`ficheros_basico.c/h`)

Inicializa las estructuras del disco y gestiona la asignacion de bloques e inodos.

| Funcion | Descripcion |
|---------|-------------|
| `initSB()` | Inicializa el superbloque con las posiciones de cada zona |
| `initMB()` | Inicializa el mapa de bits a cero (todo libre) |
| `initAI()` | Inicializa el array de inodos como lista enlazada de libres |
| `reservar_bloque()` | Busca un bit a 0 en el MB, lo pone a 1 y devuelve el bloque |
| `liberar_bloque()` | Pone a 0 el bit correspondiente en el MB |
| `reservar_inodo()` | Extrae el primer inodo de la lista de libres y lo configura |
| `liberar_inodo()` | Reinserta un inodo al inicio de la lista de libres |
| `traducir_bloque_inodo()` | Resuelve un bloque logico a bloque fisico usando punteros directos e indirectos |

La funcion `traducir_bloque_inodo` es el nucleo del direccionamiento. Segun el numero de bloque logico solicitado, accede a punteros directos (bloques 0-11), indirecto simple (12-267), doble indirecto (268-65803) o triple indirecto (65804+). Cuando se invoca con `reservar=1`, crea los bloques de indirecciones que falten sobre la marcha.

### Operaciones de fichero (`ficheros.c/h`)

Proporciona lectura, escritura, estadisticas y truncado a nivel de fichero, trabajando con numeros de inodo.

| Funcion | Descripcion |
|---------|-------------|
| `mi_write_f()` | Escribe `nbytes` desde `offset` en el fichero del inodo |
| `mi_read_f()` | Lee `nbytes` desde `offset` del fichero del inodo |
| `mi_stat_f()` | Devuelve los metadatos del inodo en una estructura `STAT` |
| `mi_chmod_f()` | Modifica los permisos del inodo |
| `mi_truncar_f()` | Trunca el fichero a `nbytes`, liberando bloques sobrantes |

`mi_write_f` y `mi_read_f` calculan que bloques fisicos corresponden al rango logico solicitado, reservando bloques nuevos si es necesario. El truncado libera bloques de forma recursiva, incluyendo los bloques de indirecciones que ya no se necesitan, con una optimizacion que salta rangos de punteros nulos sin leer disco.

### Directorios y rutas (`directorios.c/h`)

Resuelve caminos absolutos, crea y elimina entradas de directorio, y gestiona enlaces.

| Funcion | Descripcion |
|---------|-------------|
| `extraer_camino()` | Separa el primer componente de un camino del resto |
| `buscar_entrada()` | Recorre el arbol de directorios hasta encontrar una entrada |
| `mi_creat()` | Crea un nuevo fichero o directorio en la ruta indicada |
| `mi_dir()` | Lista el contenido de un directorio |
| `mi_chmod()` | Cambia permisos de un fichero por su camino |
| `mi_stat()` | Obtiene estadisticas de un fichero por su camino |
| `mi_write()` | Escribe datos en un fichero por su camino (con cache) |
| `mi_read()` | Lee datos de un fichero por su camino (con cache) |
| `mi_link()` | Crea un enlace duro (comparte inodo, incrementa `nlinks`) |
| `mi_unlink()` | Elimina una entrada de directorio y libera el inodo si `nlinks` llega a 0 |

`buscar_entrada` es la funcion central de resolucion de rutas. Recorre componente a componente desde el inodo raiz, verificando permisos de lectura en cada directorio intermedio. El parametro `reservar` determina si debe crear la entrada final cuando no existe (operaciones de creacion) o devolver error (operaciones de consulta).

### Semaforo mutex (`semaforo_mutex_posix.c/h`)

Envoltorio sobre semaforos POSIX nombrados para proteccion de secciones criticas.

| Funcion | Descripcion |
|---------|-------------|
| `initSem()` | Crea o abre el semaforo nombrado con valor inicial 1 |
| `deleteSem()` | Cierra y elimina el semaforo del sistema |
| `waitSem()` | Decrementa el semaforo (bloquea si vale 0) |
| `signalSem()` | Incrementa el semaforo (desbloquea un waiter) |

### Simulacion y verificacion (`simulacion.c/h`, `verificacion.c/h`)

Programas de prueba de concurrencia y generacion de informes.

| Programa | Descripcion |
|----------|-------------|
| `simulacion` | Lanza 100 procesos hijo, cada uno realiza 50 escrituras en posiciones aleatorias de un fichero compartido. Usa `fork()` y senales `SIGCHLD` para sincronizacion. |
| `verificacion` | Lee el fichero de registros generado por la simulacion y genera un informe con estadisticas por proceso: numero de escrituras, primera y ultima escritura, posicion menor y mayor. |

La estructura `REGISTRO` almacena fecha con microsegundos, PID del proceso, numero de escritura y posicion del registro. Cada proceso escribe sus registros de forma ordenada dentro del fichero, y la verificacion comprueba que no se han perdido escrituras ni se han solapado posiciones.

## Flujo general de funcionamiento

1. **Formateo del disco**: `mi_mkfs` crea el fichero de disco con el tamano especificado, escribe el superbloque, inicializa el mapa de bits a cero y construye la lista enlazada de inodos libres. Reserva el inodo raiz y lo marca como directorio.

2. **Montaje**: Cada comando llama a `bmount` para abrir el disco y obtener acceso a las estructuras. El semaforo se inicializa en este punto si el nivel lo requiere.

3. **Resolucion de ruta**: Para cualquier operacion sobre un camino, `buscar_entrada` recorre el arbol desde el inodo raiz, componente a componente, hasta llegar al fichero o directorio objetivo.

4. **Operacion**: Segun el comando, se lee o escribe contenido, se modifican permisos, se crean o eliminan entradas, o se obtienen estadisticas.

5. **Desmontaje**: `bumount` cierra el descriptor del disco. El semaforo se elimina con `deleteSem` cuando ya no hay procesos que lo necesiten.

## Como compilar

El sistema de build usa una variable `LEVEL` que determina que modulos se incluyen en la compilacion. Cada nivel acumula los anteriores.

```bash
# Compilar todo (nivel 13 por defecto)
make

# Compilar hasta un nivel especifico
make LEVEL=6

# Limpiar artefactos de compilacion
make clean
```

### Niveles de compilacion

| Nivel | Modulos incluidos | Programas generados |
|-------|-------------------|---------------------|
| 1-3 | Bloques, superbloque, mapa de bits, inodos | `mi_mkfs`, `leer_sf` |
| 4-6 | + Traduccion de bloques, lectura/escritura, truncado | `escribir`, `leer`, `truncar`, `permitir` |
| 7-10 | + Directorios, path I/O, enlaces | `mi_mkdir`, `mi_touch`, `mi_chmod`, `mi_ls`, `mi_link`, `mi_escribir`, `mi_cat`, `mi_stat`, `mi_rm`, `mi_rmdir`, `prueba_nivel7` |
| 11-13 | + Semaforos, simulacion, verificacion | `simulacion`, `verificacion` |

### Flags de depuracion

En el Makefile se pueden activar o desactivar las trazas de cada nivel cambiando los valores de `DEBUGN3` a `DEBUGN8`:

```makefile
CFLAGS=-c -g -Wall -std=gnu99 -DDEBUGN3=1 -DDEBUGN4=1 -DDEBUGN5=1 -DDEBUGN6=1 -DDEBUGN7=0 -DDEBUGN8=0
```

Un valor de `1` habilita los `fprintf` condicionales de ese nivel, `0` los silencia. Las trazas se envian a `stderr` y muestran informacion detallada sobre la resolucion de bloques, asignacion de inodos, liberacion de bloques y navegacion de directorios.

### Dependencias

- **gcc** con soporte para C99 (`-std=gnu99`)
- **pthread** para el enlace con `-pthread` (requerido por semaforos POSIX)
- **libreria matematica** `-lm` (enlazado estandar)
- Sistema operativo con soporte para semaforos POSIX nombrados (`/dev/shm` en Linux)

## Como ejecutar

Todos los comandos reciben como primer argumento el nombre del fichero de disco.

### 1. Crear y formatear el disco

```bash
./mi_mkfs disco 100000
```

Crea un disco virtual de 100.000 bloques (unos 100 MB). El superbloque se configura con las posiciones de cada zona y se reserva el inodo raiz.

### 2. Crear directorios

```bash
./mi_mkdir disco 7 /docs/
./mi_mkdir disco 7 /docs/pruebas/
```

El segundo argumento son los permisos en notacion octal Unix (7 = rwx). El camino debe terminar en `/` para indicar que es un directorio.

### 3. Crear ficheros vacios

```bash
./mi_touch disco 6 /docs/file.txt
```

Crea un fichero vacio con los permisos especificados.

### 4. Listar contenido

```bash
./mi_ls disco /docs/
```

Muestra las entradas del directorio con su tipo, permisos, numero de inodo, nlinks, tamano y timestamps.

### 5. Escribir en un fichero

```bash
./mi_escribir.c disco /docs/file.txt "Hola mundo" 100
```

Escribe el texto en el fichero a partir del offset 100. El offset y el tamano del texto determinan que bloques se reservan.

### 6. Leer un fichero

```bash
./mi_cat disco /docs/file.txt
```

Muestra el contenido completo del fichero por salida estandar.

### 7. Cambiar permisos

```bash
./mi_chmod disco 4 /docs/file.txt
```

Cambia los permisos del fichero a solo lectura (4 = r--).

### 8. Obtener estadisticas

```bash
./mi_stat disco /docs/file.txt
```

Muestra el tipo, permisos, timestamps, nlinks, tamano logico y bloques ocupados del fichero.

### 9. Crear enlaces duros

```bash
./mi_link disco /docs/file.txt /docs/enlace.txt
```

Crea un enlace duro que apunta al mismo inodo. Ambos caminos comparten contenido y metadatos.

### 10. Eliminar ficheros y directorios

```bash
./mi_rm disco /docs/enlace.txt
./mi_rmdir disco /docs/pruebas/
```

`mi_rm` elimina una entrada de fichero. Si era el ultimo enlace al inodo, se libera. `mi_rmdir` elimina un directorio vacio.

### 11. Simulacion de concurrencia

```bash
./simulacion
```

Lanza 100 procesos que escriben 50 registros cada uno en un fichero compartido. Genera `escritura.txt` con los registros ordenados por posicion.

### 12. Verificacion

```bash
./verificacion
```

Lee `escritura.txt` y genera `verificacion.txt` con estadisticas por proceso y un resumen global.

## Ejemplos de uso completos

### Secuencia basica de operaciones

```bash
# Compilar todo
make

# Formatear disco
./mi_mkfs disco 100000

# Crear estructura de directorios
./mi_mkdir disco 7 /docs/
./mi_mkdir disco 7 /docs/pruebas/
./mi_mkdir disco 7 /logs/

# Crear ficheros
./mi_touch disco 6 /docs/nota.txt
./mi_touch disco 6 /logs/sistema.log

# Escribir contenido
./mi_escribir.c disco /docs/nota.txt "Este es un fichero de prueba" 0
./mi_escribir.c disco /logs/sistema.log "Log de inicio del sistema" 0

# Leer contenido
./mi_cat disco /docs/nota.txt
./mi_cat disco /logs/sistema.log

# Ver estadisticas
./mi_stat disco /docs/nota.txt

# Listar directorio
./mi_ls disco /docs/

# Crear enlace duro
./mi_link disco /docs/nota.txt /docs/nota_copia.txt

# Eliminar fichero
./mi_rm disco /docs/nota_copia.txt

# Eliminar directorio vacio
./mi_rmdir disco /docs/pruebas/
```

### Ejecucion de tests

```bash
# Ejecutar todos los tests integrados
make test
```

El target `test` ejecuta secuencialmente todos los scripts de prueba: `test5a.sh`, `test5b.sh`, `test6a.sh` hasta `test6d.sh`, `scripte1.sh`, `prueba_nivel7`, `test7.sh` hasta `test10.sh`, `test_transitions.sh` y `test_permissions.sh`.

## Organizacion del repositorio

```
.
├── bloques.c/h                  # Capa de E/S de bloques (Nivel 1)
├── ficheros_basico.c/h          # Superbloque, MB, AI, alloc/free, traduccion (Niveles 2-4, 6)
├── ficheros.c/h                 # Lectura/escritura/truncado por inodo (Niveles 5-6)
├── directorios.c/h              # Directorios, rutas, enlaces (Niveles 7-10)
├── semaforo_mutex_posix.c/h     # Semaforo POSIX (Nivel 11)
├── simulacion.c/h               # Simulacion de concurrencia (Nivel 12)
├── verificacion.c/h             # Generacion de informe (Nivel 13)
│
├── mi_mkfs.c                    # Formateador del sistema de ficheros
├── mi_ls.c                      # Listado de directorios
├── mi_mkdir.c                   # Creacion de directorios
├── mi_touch.c                   # Creacion de ficheros vacios
├── mi_chmod.c                   # Cambio de permisos
├── mi_link.c                    # Creacion de enlaces duros
├── mi_rm.c                      # Eliminacion de ficheros
├── mi_rmdir.c                   # Eliminacion de directorios
├── mi_escribir.c                # Escritura en ficheros por camino
├── mi_cat.c                     # Lectura de ficheros por camino
├── mi_stat.c                    # Estadisticas de ficheros por camino
│
├── leer_sf.c                    # Lectura del superbloque
├── escribir.c                   # Escritura directa por inodo
├── leer.c                       # Lectura directa por inodo
├── truncar.c                    # Truncado por inodo
├── permitir.c                   # Cambio de permisos por inodo
├── prueba_nivel7.c              # Test de directorios
│
├── Makefile                     # Sistema de build con niveles
├── scripte1.sh                  # Test integrador niveles 1-6
├── scripte2.sh                  # Test integrador niveles 7-8
├── scripte3.sh                  # Test integrador niveles 9-10
├── test5a.sh - test6d.sh        # Tests niveles 5-6
├── test7.sh - test10.sh         # Tests niveles 7-10
├── test_permissions.sh          # Test de permisos
├── test_transitions.sh          # Test de transiciones de estado
└── docs/
    └── README_GENERAL.md        # Este documento
```

## Posibles ampliaciones

El sistema actual cubre las funcionalidades basicas de un sistema de ficheros tipo Unix. Estas son algunas lineas de extension viables:

- **Punteros cuadruple indirecto**: Anadir un cuarto nivel de indirecciones para aumentar el tamano maximo de fichero mas alla de los 16,8 millones de bloques.

- **Bloques de tamano variable**: Permitir configurar `BLOCKSIZE` en tiempo de formateo, almacenando el valor en el superbloque. Bloques mas grandes reducen la fragmentacion interna pero aumentan el desperdicio para ficheros pequenos.

- **Soporte para enlaces simbolicos**: Los enlaces duros actuales comparten inodo. Un enlace simboloico almacenaria la ruta destino como contenido del fichero, con un flag de tipo diferenciado en el inodo.

- **Journaling**: Anadir un log de transacciones antes de modificar el mapa de bits o los inodos. En caso de caida del sistema, el journal permite recuperar la consistencia sin necesidad de un `fsck` completo.

- **Cache de bloques en memoria**: Mantener un buffer cache con los bloques de datos mas accedidos, reduciendo las llamadas a `bread`/`bwrite` para bloques que ya estan en memoria.

- **Sistema de cuotas**: Limitar el numero de bloques o inodos que puede consumir un usuario o un fichero, verificando los limites en `reservar_bloque` y `reservar_inodo`.

- **Compresion de bloques**: Almacenar el contenido de los bloques de datos comprimidos, descomprimiendo en `mi_read_f` y comprimiendo en `mi_write_f`.

- **Soporte para atributos extendidos**: Anadir al inodo un area de metadatos extendidos (xattrs) para almacenar informacion arbitraria asociada al fichero, como etiquetas o hashes de integridad.
