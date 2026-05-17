# Comentarios de Diseño — Sistema de Ficheros MI_SF

**Autores:** Calvache Vidal, Antonio | Mercadal Forteza, Lluís | Salvatierra Romero, Diego Jeroni
**Asignatura:** Sistemas Operativos II (SOII)
**Fecha:** Mayo 2026
**Revisión:** Análisis técnico post-implementación

---

## 1. Puntos Fuertes

### 1.1 Arquitectura en capas bien delimitada

El sistema se organiza en cuatro capas con responsabilidades claras y dependencias unidireccionales:

| Capa | Módulo | Responsabilidad |
|------|--------|-----------------|
| E/S de bloques | `bloques.c/h` | Abstracción del disco virtual (`bmount`, `bumount`, `bread`, `bwrite`) |
| Metadatos | `ficheros_basico.c/h` | Superbloque, bitmap, inodos, direccionamiento multinivel |
| Operaciones de fichero | `ficheros.c/h` | Lectura/escritura lógica, `stat`, truncado |
| Directorios | `directorios.c/h` | Resolución de caminos, creación, enlaces, borrado |

Cada capa solo conoce la inmediatamente inferior. `directorios.h` incluye `ficheros.h`, que incluye `ficheros_basico.h`, que incluye `bloques.h`. Esta cadena de inclusión refleja fielmente la jerarquía de abstracción y evita dependencias circulares.

### 1.2 Desarrollo incremental mediante el sistema de niveles

El `Makefile` define 13 niveles de compilación (`LEVEL=1..13`). Cada nivel añade un subconjunto de ficheros fuente:

- **Niveles 1-3:** `mi_mkfs`, `leer_sf` — bases del sistema (superbloque, bitmap, array de inodos).
- **Niveles 4-6:** `escribir`, `leer`, `truncar`, `permitir` — operaciones de fichero y punteros multinivel.
- **Niveles 7-10:** `mi_mkdir`, `mi_ls`, `mi_cat`, `mi_rm`, etc. — directorios y comandos de usuario.
- **Niveles 11-13:** `simulacion`, `verificacion` — concurrencia y semaforización.

Esto permite compilar y probar cada fase de forma aislada. Un estudiante puede validar el nivel 4 sin tener implementados los directorios. Es una decisión pedagógica acertada que reduce la superficie de depuración en cada etapa.

### 1.3 Direccionamiento multinivel clásico UNIX

La estructura de inodo (`ficheros_basico.h`, líneas 56-82) implementa el esquema clásico de 12 punteros directos + 3 niveles de indirección (simple, doble, triple), con 256 punteros por bloque de 1024 bytes:

```
DIRECTOS:       12 bloques              → hasta BL 11
Indirecto 0:    256 bloques             → hasta BL 267
Indirecto 1:    65.536 bloques          → hasta BL 65.803
Indirecto 2:    16.777.216 bloques      → hasta BL 16.843.019
```

Con bloques de 1 KB, el tamaño máximo de fichero es ~16 GB. La función `traducir_bloque_inodo()` (`ficheros_basico.c`, línea 785) resuelve la navegación iterativa por el árbol de punteros, creando bloques índice bajo demanda cuando `reservar=1`. La separación en `obtener_nRangoBL()` y `obtener_indice()` mantiene la función principal legible.

### 1.4 Liberación recursiva de bloques con optimización

`liberar_indirectos_recursivo()` (`ficheros_basico.c`, línea 965) es probablemente la función más sofisticada del proyecto. Implementa dos optimizaciones significativas:

1. **Salto de rangos vacíos:** Si un puntero es 0 o todo el subárbol que representa queda antes del punto de truncamiento (`*nBL + saltos <= primerBL`), la función retorna inmediatamente sin leer el bloque de disco. Esto evita lecturas innecesarias en ficheros dispersos.

2. **Liberación de bloques índice vacíos:** Tras liberar los hijos, se compara el bloque de punteros con un buffer de ceros mediante `memcmp()`. Si todos los punteros son 0, el propio bloque índice se libera. Esto evita dejar "bloques huérfanos" que ocuparían espacio sin apuntar a nada.

Los contadores `cbreads` y `cbwrites` (`ficheros_basico.c`, líneas 21-22) permiten cuantificar el impacto de estas optimizaciones en los tests.

### 1.5 Caché de última entrada resuelta

`directorios.c` define dos variables estáticas `UltimaEntradaLectura` y `UltimaEntradaEscritura` (líneas 29-30) que almacenan el camino resuelto y su inodo correspondiente. Antes de invocar la costosa `buscar_entrada()`, las funciones de nivel superior (`mi_write`, `mi_read`, etc.) comprueban si el camino coincide con la caché. Para patrones de acceso secuencial (lectura/escritura repetida al mismo fichero), esto elimina completamente la resolución de caminos.

La separación en dos cachés (lectura/escritura) evita que una operación de lectura invalide el resultado de una escritura reciente, aunque en la práctica ambas podrían colisionar si se alternan operaciones sobre el mismo fichero.

### 1.6 Semaforización POSIX con reentrada

`semaforo_mutex_posix.c` implementa un semáforo con nombre (`/sem_mi_sf`) con valor inicial 1, actuando como mutex. La variable `inside_sc` (`bloques.c`, línea 24) lleva la cuenta de las entradas anidadas en la sección crítica, permitiendo reentrada. Esto es crucial porque funciones como `mi_write_f()` llaman a `traducir_bloque_inodo()`, que a su vez llama a `reservar_bloque()`, y todas intentan adquirir el semáforo.

El cierre del descriptor heredado tras `fork()` en `bmount()` (`bloques.c`, líneas 42-44) es una decisión correcta: evita que padre e hijos compartan el mismo puntero de fichero (`lseek`), lo que causaría corrupción de datos.

### 1.7 Documentación inline consistente

Cada función sigue el formato de cabecera prescrito: parámetros, retorno, funciones que llama, funciones que la llaman. Los comentarios de autoría en cada fichero documentan quién programó, revisó y verificó. Esto facilita la trazabilidad de errores y la revisión por pares.

### 1.8 Salida de depuración con colores ANSI

Las macros de color en `bloques.h` (líneas 34-47) y los bloques `#if DEBUGN3` ... `#endif` permiten activar trazas específicas por nivel de compilación. Por ejemplo, `DEBUGN6` muestra el detalle de la liberación de bloques con información de bloques lógicos y físicos. La configuración en el `Makefile` (`-DDEBUGN3=1 -DDEBUGN4=1 ...`) permite activar/desactivar trazas sin modificar el código.

---

## 2. Simplificaciones

### 2.1 Estructura plana de ficheros

Todos los `.c` y `.h` coexisten en el directorio raíz. No hay subdirectorios `src/`, `include/`, `tests/`. Para un proyecto de ~25 ficheros fuente es aceptable, pero dificulta la navegación y la construcción de sistemas de build más complejos. Un `Makefile` con 56 reglas de compilación en un solo directorio es difícil de mantener si el proyecto creciera.

### 2.2 Ausencia de journaling

No existe ningún mecanismo de journaling o registro de transacciones. Si el sistema se interrumpe durante una operación de escritura (corte de energía, `kill -9`), el filesystem puede quedar en un estado inconsistente: bloques marcados como ocupados en el bitmap pero no referenciados por ningún inodo, o inodos con punteros a bloques no inicializados. La recuperación requeriría un `fsck` completo que escanee todo el bitmap y el array de inodos.

### 2.3 Modelo de permisos reducido

Los permisos se almacenan en un único `unsigned char` (`inodo.permisos`), lo que permite solo 8 combinaciones (0-7 en octal). No hay distinción entre propietario, grupo y otros. La verificación de permisos en `mi_write_f()` (`ficheros.c`, línea 58) comprueba simplemente `(inodo.permisos & 2) != 2`, sin considerar quién es el usuario que realiza la operación. En la práctica, todos los usuarios tienen los mismos permisos sobre un fichero.

### 2.4 Entradas de directorio de tamaño fijo

Cada entrada ocupa 64 bytes: 60 para el nombre + 4 para el número de inodo (`directorios.h`, líneas 25-29). Para nombres cortos como "a.txt" (5 bytes), se desperdician 55 bytes por entrada. En un bloque de 1024 bytes caben exactamente 16 entradas. Un directorio con 1000 ficheros ocupa ~64 KB independientemente de la longitud real de los nombres.

### 2.5 Ausencia de enlaces simbólicos

Solo se soportan enlaces duros (`mi_link`/`mi_unlink`). No hay mecanismo para crear enlaces simbólicos que apunten a caminos relativos o absolutos. Esto limita la flexibilidad del sistema de ficheros: no se pueden crear atajos a directorios, ni referencias cruzadas entre árboles de directorios sin incrementar el contador de enlaces del inodo destino.

### 2.6 Semáforo global único

Toda la concurrencia se gestiona con un único semáforo mutex (`/sem_mi_sf`). Dos procesos que operen sobre ficheros completamente distintos (inodos diferentes, bloques diferentes) se serializan innecesariamente. En `simulacion.c`, 100 procesos que escriben en sus propios directorios `/simul_.../proceso_PID/` se ejecutan secuencialmente, no en paralelo.

### 2.7 Sin caché de bloques de disco

Cada operación de lectura o escritura accede directamente al disco mediante `bread()`/`bwrite()`. No existe un buffer cache que retenga bloques recientemente leídos en memoria. Si un proceso lee el superbloque 100 veces (una por cada `leer_inodo()`), se realizan 100 lecturas de disco reales. La función `leer_inodo()` (`ficheros_basico.c`, línea 565) lee el superbloque en cada invocación para calcular la posición del bloque AI, cuando el superbloque rara vez cambia.

### 2.8 Superbloque único sin replicación

El superbloque reside exclusivamente en el bloque 0 (`posSB = 0`). No hay copias de respaldo. Si el bloque 0 se corrompe, se pierde toda la información de layout del disco: posiciones del bitmap, array de inodos y zona de datos. Los filesystems reales (ext4, XFS) replican el superbloque en múltiples ubicaciones.

---

## 3. Modularidad

### 3.1 Acoplamiento entre capas

El acoplamiento es **unidireccional y descendente**, lo cual es correcto:

```
directorios → ficheros → ficheros_basico → bloques
```

Sin embargo, hay **acoplamiento encubierto** en varios puntos:

- `buscar_entrada()` (`directorios.c`) llama directamente a `mi_read_f()` y `mi_write_f()` de la capa de ficheros, pero también accede a `reservar_inodo()` y `liberar_inodo()` de la capa de metadatos. Esto cruza una capa de abstracción.
- `liberar_bloques_inodo()` (`ficheros_basico.c`) es llamada tanto por `mi_truncar_f()` (capa ficheros) como por `liberar_inodo()` (capa metadatos), lo que crea una dependencia circular potencial si no se gestiona con cuidado.

### 3.2 Cohesión interna

Cada módulo tiene **cohesión funcional alta**:

- `bloques.c`: solo se ocupa de E/S a nivel de bloque.
- `ficheros_basico.c`: gestiona metadatos (superbloque, bitmap, inodos, traducción de bloques).
- `ficheros.c`: operaciones de lectura/escritura lógica sobre inodos.
- `directorios.c`: resolución de caminos y manipulación de entradas.

La excepción es `ficheros_basico.c`, que mezcla la gestión de metadatos (Niveles 2-3) con el direccionamiento multinivel (Nivel 4) y la liberación de bloques (Nivel 6). Sería más limpio separar `traducir_bloque_inodo()` y las funciones de liberación en un módulo de "direccionamiento".

### 3.3 Impacto de cambios entre capas

Un cambio en la capa inferior **propaga hacia arriba**:

- Modificar `BLOCKSIZE` afectaría a `NPUNTEROS`, `INODOSIZE`, los umbrales de indirección, el tamaño de entradas de directorio, y todos los cálculos de offset.
- Cambiar la estructura `struct inodo` requeriría ajustar el `padding` y recalcular `tamAI()`.
- Añadir un nuevo nivel de indirección implicaría modificar `obtener_nRangoBL()`, `obtener_indice()`, `traducir_bloque_inodo()` y `liberar_indirectos_recursivo()`.

Las interfaces entre capas son **explícitas** (prototipos en `.h`), pero no hay validación de precondiciones en los límites de capa. Por ejemplo, `traducir_bloque_inodo()` asume que el inodo existe y es válido; si se le pasa un `ninodo` fuera de rango, el comportamiento es indefinido.

---

## 4. Reutilización

### 4.1 Patrón bmount → operación → bumount

Todos los comandos de usuario siguen el mismo patrón estructural:

```c
bmount(argv[1]);
// ... operación ...
bumount();
```

Este patrón se repite en `mi_mkfs.c`, `leer_sf.c`, `escribir.c`, `mi_ls.c`, `mi_cat.c`, `mi_mkdir.c`, `mi_rm.c`, etc. Sería posible factorizarlo en una función wrapper, pero la separación en programas independientes es intencional: cada comando es un proceso independiente que monta y desmonta el disco.

### 4.2 Reutilización de funciones base

Las funciones de nivel inferior se reutilizan extensivamente:

| Función | Llamada desde |
|---------|--------------|
| `traducir_bloque_inodo()` | `mi_write_f()`, `mi_read_f()` |
| `buscar_entrada()` | `mi_creat()`, `mi_chmod()`, `mi_stat()`, `mi_write()`, `mi_read()`, `mi_link()`, `mi_unlink()` |
| `mi_write_f()` | `mi_write()`, `buscar_entrada()` (para crear entradas), `simulacion.c` |
| `mi_read_f()` | `mi_read()`, `buscar_entrada()` (para leer bloques de directorio) |
| `liberar_bloques_inodo()` | `mi_truncar_f()`, `liberar_inodo()` |

`buscar_entrada()` es el ejemplo más notable: una sola función recursiva resuelve caminos para creación, consulta, modificación y borrado. El parámetro `reservar` (0=solo buscar, 1=crear si no existe) controla el comportamiento sin duplicar lógica.

### 4.3 Diseño tipo biblioteca

Los módulos `bloques`, `ficheros_basico`, `ficheros` y `directorios` se compilan como objetos `.o` reutilizables. Los comandos de usuario los enlazan según el nivel. Esto permite que un nuevo comando (p.ej., `mi_cp`) se implemente simplemente incluyendo las cabeceras apropiadas y llamando a las funciones existentes, sin modificar el código base.

---

## 5. Mantenibilidad

### 5.1 Legibilidad del código

Los identificadores en español (`reservar_bloque`, `buscar_entrada`, `tamEnBytesLog`) son descriptivos y consistentes. La indentación es uniforme y los bloques `if/else` están bien estructurados. El uso de `goto _fin` para la gestión de errores en `mi_write_f()` y `mi_read_f()` es una práctica aceptable en C que evita la duplicación de `mi_signalSem()` en cada rama de error.

### 5.2 Calidad de los comentarios

Los comentarios de cabecera de función son **excelentes**: documentan parámetros, retorno, funciones llamadas y funciones llamadoras. Los comentarios inline explican el "por qué" más que el "qué", lo cual es la práctica correcta.

Sin embargo, hay comentarios que reflejan decisiones de diseño dudosas sin justificarlas. Por ejemplo, en `liberar_bloque()` (`ficheros_basico.c`, líneas 480-481):

> *"NOTA: Se podría optimizar (la función abre el SB 2 veces...) pero se ha decidido mantener la simplicidad."*

Este comentario reconoce una ineficiencia conocida pero no la corrige. Es honesto, pero indica trabajo pendiente.

### 5.3 Gestión de errores consistente

El sistema usa dos esquemas de error que **no se integran**:

1. **`EXITO`/`FALLO`** (0/-1) en `bloques.h` para errores de bajo nivel.
2. **Códigos negativos específicos** en `directorios.h` (`ERROR_CAMINO_INCORRECTO = -1`, `ERROR_PERMISO_LECTURA = -2`, etc.).

El problema es que `ERROR_CAMINO_INCORRECTO` vale `-1`, que coincide con `FALLO`. Una función que retorna `-1` puede significar "camino incorrecto" o "error genérico de E/S", y el llamador no puede distinguirlos sin conocer el contexto. La función `mostrar_error_buscar_entrada()` (`directorios.c`, línea 261) ayuda a depurar pero no resuelve la ambigüedad programática.

### 5.4 Convenciones de nomenclatura

- **Funciones:** `snake_case` en español (`reservar_bloque`, `buscar_entrada`).
- **Constantes:** `MAYÚSCULAS` (`BLOCKSIZE`, `EXITO`, `NPUNTEROS`).
- **Estructuras:** `snake_case` con `struct` explícito (`struct inodo`, `struct entrada`).
- **Variables globales de debug:** `cbreads`, `cbwrites` (abreviaturas poco claras).

La convención es consistente en todo el proyecto. El uso de `p_` como prefijo para punteros de salida (`p_inodo`, `p_entrada`, `p_stat`) es una buena práctica que documenta la semántica del parámetro.

---

## 6. Complejidad

### 6.1 Funciones más complejas

| Función | Líneas | Complejidad ciclomática estimada | Razón |
|---------|--------|-----------------------------------|-------|
| `traducir_bloque_inodo()` | ~117 | ~15 | Bucle while con 4 ramas (ptr==0, reservar, nivel, rango) + escritura condicional |
| `liberar_indirectos_recursivo()` | ~108 | ~18 | Recursión + 3 niveles de indirección + optimización de salto + memcmp |
| `buscar_entrada()` | ~177 | ~14 | Recursión + búsqueda lineal + 7 códigos de error + creación de entradas |
| `reservar_bloque()` | ~89 | ~10 | Bucle anidado (bloque → byte → bit) + múltiples puntos de salida |
| `mi_write_f()` | ~129 | ~12 | 3 casos (un bloque, múltiples bloques parciales, bloques intermedios) + permisos + metadatos |

### 6.2 Gestión de la complejidad

El proyecto maneja la complejidad mediante:

1. **Descomposición funcional:** `traducir_bloque_inodo()` delega en `obtener_nRangoBL()` y `obtener_indice()`, reduciendo su complejidad interna.
2. **Recursión controlada:** `buscar_entrada()` y `liberar_indirectos_recursivo()` usan recursión con caso base claro (camino vacío o nivel 1). La profundidad máxima está acotada por `PROFUNDIDAD = 32`.
3. **Flags de debug condicionales:** Los bloques `#if DEBUGN6` permiten activar trazas solo cuando se necesita depurar funciones complejas, sin afectar el rendimiento en producción.

La función `obtener_indice()` (`ficheros_basico.c`, línea 723) es un ejemplo de complejidad innecesaria: usa una cascada de `if/else` con cálculos aritméticos diferentes para cada nivel de indirección. Podría simplificarse con un bucle que aplique la misma fórmula iterativamente.

### 6.3 Complejidad de `liberar_indirectos_recursivo()`

Esta función es la más compleja del proyecto por varias razones:

- Mantiene un contador global `*nBL` que se comparte entre llamadas recursivas.
- Calcula `saltos` (el número de bloques lógicos que abarca un subárbol) dinámicamente según el nivel.
- Tiene dos condiciones de salto temprano (puntero vacío o rango anterior al punto de truncamiento).
- Distingue entre caso base (nivel 1, apunta a datos) y caso recursivo (nivel > 1, apunta a punteros).
- Tras la recursión, decide si liberar el bloque índice (memcmp con ceros) o escribirlo (modificado pero no vacío).

La variable `eof` actúa como señal de terminación temprana que se propaga hacia arriba en la pila de llamadas. Es un patrón correcto pero difícil de seguir sin las trazas de debug.

---

## 7. Posibles Mejoras

### 7.1 Caché de bloques de disco

**Problema:** Cada `leer_inodo()` lee el superbloque para calcular la posición del bloque AI. En una operación típica como `mi_write_f()`, se leen múltiples inodos y bloques de punteros, cada uno requiriendo lecturas de disco independientes.

**Propuesta:** Implementar un buffer cache con política LRU que retenga los bloques más recientemente accedidos. El superbloque, al ser leído en casi todas las operaciones, debería estar siempre en caché. Un cache de 16-32 bloques reduciría drásticamente las operaciones de E/S.

### 7.2 Enlaces simbólicos

**Problema:** Solo se soportan enlaces duros. No hay forma de crear referencias simbólicas a caminos.

**Propuesta:** Añadir un nuevo tipo de inodo `'s'` (simbólico) cuyo contenido sea el camino destino. Las funciones de resolución de caminos deberían detectar este tipo y seguir la referencia recursivamente, con un límite de profundidad para evitar bucles.

### 7.3 Modelo de permisos completo (Unix)

**Problema:** Un solo byte de permisos sin distinción de usuario/grupo/otros.

**Propuesta:** Expandir `permisos` a 9 bits (rwx para owner, group, other) más bits especiales (setuid, setgid, sticky). Esto requeriría almacenar el UID del propietario en el inodo y comparar con el usuario efectivo del proceso.

### 7.4 Journaling para recuperación ante fallos

**Problema:** Sin journaling, una interrupción durante una escritura puede dejar el filesystem inconsistente.

**Propuesta:** Implementar un journal simple (write-ahead logging) que registre las operaciones antes de ejecutarlas. Al montar el disco, si el journal tiene entradas pendientes, se replayan o se descartan. Un journal circular de 64 bloques sería suficiente para la mayoría de operaciones.

### 7.5 Búsqueda optimizada en directorios

**Problema:** `buscar_entrada()` realiza búsqueda lineal en las entradas del directorio. Para un directorio con 10.000 ficheros, cada búsqueda requiere leer ~625 bloques de directorio.

**Propuesta:** Implementar un índice B-tree o una tabla hash para directorios grandes. Para directorios pequeños (< 16 entradas), mantener la búsqueda lineal. La transición podría ser automática cuando el directorio supera un umbral.

### 7.6 Semáforos de grano fino

**Problema:** Un único semáforo global serializa todas las operaciones.

**Propuesta:** Implementar un semáforo por inodo o por bloque de datos. Dos procesos que operen sobre ficheros diferentes podrían ejecutarse en paralelo. Un hash de `ninodo` a semáforo permitiría tener N semáforos (p.ej., 64) sin overhead excesivo.

### 7.7 Ficheros dispersos (sparse files)

**Problema:** `mi_read_f()` ya soporta lectura de huecos (rellena con ceros cuando `traducir_bloque_inodo()` retorna -1), pero `mi_write_f()` siempre reserva bloques. No hay mecanismo para crear ficheros dispersos eficientemente.

**Propuesta:** Añadir una operación `mi_punch_hole()` que libere bloques en un rango intermedio sin truncar el fichero. Esto permitiría crear ficheros con huecos sin ocupar bloques físicos.

### 7.8 Replicación del superbloque

**Problema:** Un único superbloque en el bloque 0 es un punto único de fallo.

**Propuesta:** Guardar copias del superbloque en bloques estratégicos (p.ej., al inicio de cada grupo de 1024 bloques de datos). Al montar, leer la primera copia válida.

---

## 8. Escalabilidad

### 8.1 Escalabilidad con más bloques

El bitmap escala linealmente: cada bloque adicional requiere 1 bit más. Para 100.000 bloques (disco de ~100 MB), el bitmap ocupa ~13 bloques. Para 1.000.000 de bloques (~1 GB), ocuparía ~122 bloques. La búsqueda lineal del primer bit libre en `reservar_bloque()` (`ficheros_basico.c`, línea 383) escanea bloques del MB secuencialmente. Con `memcmp(bufferMB, bufferAux, BLOCKSIZE)` se saltan bloques completos llenos, lo cual es eficiente mientras haya espacio libre. Cuando el disco está casi lleno, la degradación es lineal: O(n bloques de bitmap).

### 8.2 Escalabilidad con más inodos

La lista enlazada de inodos libres es LIFO: el último inodo liberado es el primero en reutilizarse. Esto es O(1) para reserva y liberación. Sin embargo, no hay forma de reservar un inodo específico; siempre se toma el primero de la lista. Si se liberan inodos dispersos, la lista los reordena, pero esto no afecta al rendimiento.

El límite práctico es el tamaño del array de inodos: con 100.000 bloques de disco, el `mi_mkfs` calcula heurísticamente el número de inodos. Si se necesitan más ficheros que inodos disponibles, no se pueden crear más, aunque haya espacio en disco.

### 8.3 Limitaciones de directorios grandes

La búsqueda lineal en directorios es el cuello de botella más significativo. Cada entrada ocupa 64 bytes, y se leen bloques de 1024 bytes (16 entradas por bloque). Para un directorio con N entradas:

- **Mejor caso:** 1 lectura de bloque (entrada en primer bloque).
- **Peor caso:** N/16 lecturas de bloque (entrada al final o no existe).
- **Caso medio:** N/32 lecturas de bloque.

Con 10.000 entradas, una búsqueda promedio requiere ~156 lecturas de bloque de directorio. Si cada lectura implica un `bread()` real (sin caché), el coste es significativo.

### 8.4 Limitaciones de concurrencia

El semáforo global impone un límite duro: **un solo proceso puede operar a la vez**, independientemente de cuántos procesos estén esperando. En `simulacion.c`, 100 procesos se ejecutan secuencialmente. El tiempo total de la simulación es la suma de los tiempos individuales, no el máximo.

Con un sistema de semáforos por inodo, los 100 procesos podrían ejecutarse en paralelo (cada uno opera sobre su propio directorio), reduciendo el tiempo total al del proceso más lento.

### 8.5 Tamaño máximo del filesystem

Con los parámetros actuales (BLOCKSIZE=1024, 12 directos + 3 niveles de indirección):

| Métrica | Valor |
|---------|-------|
| Máximo fichero | ~16.843.020 × 1024 ≈ 16 GB |
| Máximo bloques | Limitado por el bitmap (UINT_MAX bits ≈ 512 GB) |
| Máximo inodos | Limitado por el array de inodos (heuristicamente calculado) |
| Máximo profundidad de camino | `PROFUNDIDAD = 32` |
| Máximo longitud de nombre | `TAMNOMBRE = 60` |

El límite de 32 niveles de profundidad es generoso para un filesystem educativo. El límite de 60 caracteres para nombres es razonable pero podría ser insuficiente para paths generados automáticamente.

---

## Resumen

MI_SF es un filesystem educativo bien estructurado que implementa correctamente los conceptos fundamentales de un sistema de ficheros tipo UNIX: inodos, direccionamiento multinivel, bitmap de bloques libres, directorios con resolución de caminos, y concurrencia básica con semáforos. Las decisiones de diseño priorizan la **claridad pedagógica** sobre la **eficiencia productiva**, lo cual es apropiado para un proyecto de SOII.

Las principales áreas de mejora —caché de bloques, journaling, permisos completos y semáforos de grano fino— son extensiones naturales que transformarían MI_SF de un filesystem educativo a uno funcional para uso real.
