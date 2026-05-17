# Guia de Uso del Sistema de Ficheros Virtual

Sistema de ficheros basado en inodos implementado en C sobre un disco virtual. Proyecto de la asignatura Sistemas Operativos II (SOII).

---

## 1. Introduccion

Este proyecto implementa un sistema de ficheros completo en espacio de usuario que opera sobre un fichero que actua como disco virtual. El sistema incluye gestion de superbloque, mapa de bits, array de inodos, punteros directos e indirectos (simple, doble y triple), directorios con arbol jerarquico, enlaces fisicos, control de permisos y soporte para concurrencia con semaforos POSIX.

**Que puedes hacer con este sistema de ficheros:**

- Formatear y crear un disco virtual desde cero
- Crear directorios y ficheros con permisos
- Escribir y leer contenido en ficheros
- Listar el contenido de directorios
- Crear enlaces fisicos (hard links)
- Cambiar permisos de lectura, escritura y ejecucion
- Truncar ficheros a un tamaño determinado
- Ejecutar simulaciones de concurrencia con 100 procesos
- Diagnosticar la estructura interna del disco

---

## 2. Comandos Disponibles

### Comandos de alto nivel (interfaz tipo shell)

| Comando | Descripcion |
|---------|-------------|
| `mi_mkfs` | Formatea el disco virtual |
| `mi_mkdir` | Crea un directorio |
| `mi_touch` | Crea un fichero vacio |
| `mi_ls` | Lista contenido de directorio o fichero |
| `mi_chmod` | Cambia permisos por ruta |
| `mi_stat` | Muestra metadatos de inodo por ruta |
| `mi_escribir` | Escribe texto en un fichero |
| `mi_cat` | Muestra contenido de un fichero |
| `mi_link` | Crea un enlace fisico |
| `mi_rm` | Elimina fichero o directorio |
| `mi_rmdir` | Elimina un directorio vacio |

### Comandos de diagnostico y pruebas

| Comando | Descripcion |
|---------|-------------|
| `leer_sf` | Muestra informacion del superbloque |
| `escribir` | Escrituras de prueba en multiples offsets |
| `leer` | Lee un fichero por numero de inodo |
| `truncar` | Trunca un fichero por numero de inodo |
| `permitir` | Cambia permisos por numero de inodo |
| `simulacion` | Ejecuta test de concurrencia con 100 procesos |
| `verificacion` | Verifica resultados de una simulacion |

---

## 3. Sintaxis Detallada

### mi_mkfs

```
./mi_mkfs <disco> <nbloques>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del fichero que sera el disco virtual (ej. `disco`) |
| `nbloques` | Numero total de bloques de 1024 bytes |

Formatea el disco virtual. Inicializa el superbloque, el mapa de bits, el array de inodos y reserva el inodo raiz. El numero de inodos se calcula como `nbloques / 4`.

**Ejemplo:** `./mi_mkfs disco 100000` crea un disco de ~100 MB con 25000 inodos.

---

### mi_mkdir

```
./mi_mkdir <disco> <permisos> </ruta/directorio/>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del disco virtual |
| `permisos` | Permisos en octal (0 a 7) |
| `ruta` | Ruta absoluta del directorio. **Debe terminar en `/`** |

Crea un directorio con los permisos indicados. La ruta debe existir en todos sus niveles intermedios.

**Ejemplo:** `./mi_mkdir disco 7 /docs/`

---

### mi_touch

```
./mi_touch <disco> <permisos> <ruta/fichero>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del disco virtual |
| `permisos` | Permisos en octal (0 a 7) |
| `ruta` | Ruta absoluta del fichero. **NO debe terminar en `/`** |

Crea un fichero vacio con los permisos indicados.

**Ejemplo:** `./mi_touch disco 6 /docs/notas.txt`

---

### mi_ls

```
./mi_ls [-l] <disco> <ruta>
```

| Argumento | Descripcion |
|-----------|-------------|
| `-l` | Opcional. Muestra listado detallado con tipo, permisos, fecha, tamaño y nombre |
| `disco` | Nombre del disco virtual |
| `ruta` | Ruta del directorio (termina en `/`) o fichero (sin `/` final) |

Lista el contenido de un directorio o muestra informacion de un fichero.

**Ejemplos:**
```
./mi_ls disco /docs/
./mi_ls -l disco /docs/
```

---

### mi_chmod

```
./mi_chmod <disco> <permisos> <ruta>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del disco virtual |
| `permisos` | Nuevos permisos en octal (0 a 7) |
| `ruta` | Ruta del fichero o directorio |

Cambia los permisos de un fichero o directorio existente.

**Ejemplo:** `./mi_chmod disco 4 /docs/notas.txt`

---

### mi_stat

```
./mi_stat <disco> <ruta>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del disco virtual |
| `ruta` | Ruta del fichero o directorio |

Muestra los metadatos del inodo asociado a la ruta: numero de inodo, tipo, permisos, timestamps (atime, mtime, ctime), numero de enlaces, tamaño logico en bytes y bloques ocupados.

**Ejemplo:** `./mi_stat disco /docs/notas.txt`

---

### mi_escribir

```
./mi_escribir <disco> <ruta> <texto> <offset>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del disco virtual |
| `ruta` | Ruta del fichero |
| `texto` | Cadena de texto a escribir (entre comillas si tiene espacios) |
| `offset` | Posicion en bytes desde el inicio donde empezar a escribir |

Escribe texto en un fichero a partir del offset indicado. Si el offset esta mas alla del tamaño actual, se rellena con ceros hasta esa posicion.

**Ejemplo:** `./mi_escribir disco /docs/notas.txt "Hola mundo" 0`

---

### mi_cat

```
./mi_cat <disco> <ruta>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del disco virtual |
| `ruta` | Ruta del fichero a mostrar |

Lee y muestra por pantalla todo el contenido de un fichero.

**Ejemplo:** `./mi_cat disco /docs/notas.txt`

---

### mi_link

```
./mi_link <disco> <ruta_origen> <ruta_destino>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del disco virtual |
| `ruta_origen` | Ruta del fichero existente (no debe terminar en `/`) |
| `ruta_destino` | Ruta del nuevo enlace (no debe existir, no debe terminar en `/`) |

Crea un enlace fisico. Ambos caminos comparten el mismo inodo y los mismos datos. Incrementa el contador de enlaces (`nlinks`) del inodo. Solo funciona con ficheros, no con directorios.

**Ejemplo:** `./mi_link disco /docs/notas.txt /docs/notas_link`

---

### mi_rm

```
./mi_rm <disco> <ruta>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del disco virtual |
| `ruta` | Ruta del fichero o directorio a eliminar |

Elimina una entrada del directorio. Si es el ultimo enlace fisico del inodo, se liberan todos sus bloques de datos. No se puede borrar el directorio raiz (`/`).

**Ejemplo:** `./mi_rm disco /docs/notas_link`

---

### mi_rmdir

```
./mi_rmdir <disco> <ruta/directorio/>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del disco virtual |
| `ruta` | Ruta del directorio a eliminar. **Debe terminar en `/`** |

Elimina un directorio vacio. Si el directorio contiene entradas, se rechaza la operacion.

**Ejemplo:** `./mi_rmdir disco /docs/trabajo/`

---

### leer_sf

```
./leer_sf <disco>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del disco virtual |

Muestra informacion del superbloque: numero total de bloques, bloques del mapa de bits, bloques del array de inodos, bloques de datos disponibles, total de inodos, inodos libres, bloques libres y tamaño del bloque. Util para diagnosticar el estado del disco.

---

### escribir

```
./escribir <disco> <"texto"> <diferentes_inodos>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del disco virtual |
| `texto` | Cadena a escribir |
| `diferentes_inodos` | `0` = mismo inodo para todos los offsets, `1` = inodo distinto por offset |

Escribe el texto en 5 offsets predefinidos (9000, 209000, 30725000, 409605000, 480000000) para probar todos los niveles de punteros del inodo: directos, indirecto simple, indirecto doble e indirecto triple. Herramienta de prueba y demostracion.

---

### leer

```
./leer <disco> <ninodo>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del disco virtual |
| `ninodo` | Numero del inodo a leer |

Lee secuencialmente todo el contenido de un fichero identificado por su numero de inodo y lo vuelca por pantalla.

---

### truncar

```
./truncar <disco> <ninodo> <nbytes>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del disco virtual |
| `ninodo` | Numero del inodo a truncar |
| `nbytes` | Nuevo tamaño en bytes. Si es `0`, se libera el inodo completo |

Reduce el tamaño de un fichero. Libera los bloques de datos que ya no son necesarios. Si `nbytes` es 0, se elimina completamente el inodo y todos sus bloques.

---

### permitir

```
./permitir <disco> <ninodo> <permisos>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del disco virtual |
| `ninodo` | Numero del inodo |
| `permisos` | Nuevos permisos (0 a 7) |

Cambia los permisos de un inodo directamente por su numero, sin necesidad de conocer su ruta.

---

### simulacion

```
./simulacion <disco>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del disco virtual |

Lanza 100 procesos hijos que escriben concurrentemente en ficheros dentro de un directorio de simulacion con nombre `/simul_aaaammddhhmmss/`. Cada proceso realiza 100 escrituras. Usa semaforos POSIX para sincronizacion. Al terminar, muestra el numero de procesos finalizados.

---

### verificacion

```
./verificacion <disco> </ruta/simulacion/>
```

| Argumento | Descripcion |
|-----------|-------------|
| `disco` | Nombre del disco virtual |
| `ruta` | Ruta del directorio de simulacion generado por `simulacion` |

Recorre los directorios de cada proceso dentro de la simulacion, analiza los ficheros `prueba.dat` y genera un `informe.txt` con estadisticas de coherencia: registros esperados vs encontrados, bytes escritos, tiempos de ejecucion y deteccion de errores.

---

## 4. Ejemplos de Uso

### Paso 1: Formatear el disco

```bash
./mi_mkfs disco 100000
```

Salida esperada:
```
Inicializando disco 'disco' con 100000 bloques...
disco montado
Formateo completado. Inodo raiz reservado: 0
```

### Paso 2: Crear directorios

```bash
./mi_mkdir disco 7 /docs/
./mi_mkdir disco 7 /docs/trabajo/
```

El permiso `7` otorga lectura, escritura y ejecucion. Los directorios deben terminar en `/`.

### Paso 3: Crear ficheros

```bash
./mi_touch disco 6 /docs/trabajo/notas.txt
```

El permiso `6` otorga lectura y escritura. Los ficheros NO terminan en `/`.

### Paso 4: Escribir contenido

```bash
./mi_escribir disco /docs/trabajo/notas.txt "Hola mundo" 0
```

Salida:
```
Longitud texto: 10
Bytes escritos: 10
```

Puedes escribir en diferentes posiciones:

```bash
./mi_escribir disco /docs/trabajo/notas.txt " Segunda linea" 10
```

### Paso 5: Leer contenido

```bash
./mi_cat disco /docs/trabajo/notas.txt
```

Salida:
```
Hola mundo Segunda linea
```

### Paso 6: Listar directorios

Listado simple:
```bash
./mi_ls disco /docs/
```

Listado detallado:
```bash
./mi_ls -l disco /docs/
```

Salida:
```
Total: 1
Tipo    Modo    mTime                   Tamaño  Nombre
--------------------------------------------------------------------------------
d       7       2025-05-07 10:30:00     64      trabajo/
```

### Paso 7: Crear un enlace fisico

```bash
./mi_link disco /docs/trabajo/notas.txt /docs/notas_link
```

Ambos caminos apuntan al mismo inodo. Si modificas uno, se refleja en el otro.

### Paso 8: Cambiar permisos

```bash
./mi_chmod disco 4 /docs/trabajo/notas.txt
```

El fichero queda en modo solo lectura (4 = lectura).

### Paso 9: Ver metadatos

```bash
./mi_stat disco /docs/trabajo/notas.txt
```

Salida:
```
Nº de inodo: 5
tipo: f
permisos: 4
atime: 2025-05-07 10:30:00
mtime: 2025-05-07 10:30:00
ctime: 2025-05-07 10:30:00
nlinks: 2
tamEnBytesLog: 24
numBloquesOcupados: 1
```

### Paso 10: Eliminar fichero

```bash
./mi_rm disco /docs/notas_link
```

Solo se decrementa `nlinks`. El fichero original sigue existiendo.

### Paso 11: Eliminar directorio vacio

```bash
./mi_rmdir disco /docs/trabajo/
```

El directorio debe estar vacio. Si contiene ficheros, primero eliminalos con `mi_rm`.

### Paso 12: Ejecutar simulacion de concurrencia

```bash
./simulacion disco
```

Esto crea un directorio como `/simul_20250507103000/` con 100 subdirectorios de proceso.

### Paso 13: Verificar resultados

```bash
./verificacion disco /simul_20250507103000/
```

Genera un `informe.txt` dentro del directorio de simulacion con las estadisticas.

---

## 5. Casos Comunes

### Crear un arbol de directorios

```bash
./mi_mkfs disco 100000
./mi_mkdir disco 7 /proyecto/
./mi_mkdir disco 7 /proyecto/src/
./mi_mkdir disco 7 /proyecto/docs/
./mi_mkdir disco 7 /proyecto/docs/borradores/
```

Verifica la estructura:
```bash
./mi_ls -l disco /proyecto/
```

### Escribir y leer ficheros

```bash
./mi_touch disco 6 /proyecto/src/main.c
./mi_escribir disco /proyecto/src/main.c "#include <stdio.h>\n" 0
./mi_escribir disco /proyecto/src/main.c "int main() { return 0; }\n" 16
./mi_cat disco /proyecto/src/main.c
```

### Usar enlaces fisicos

Los enlaces fisicos son utiles cuando quieres acceder al mismo fichero desde diferentes directorios sin duplicar datos:

```bash
./mi_touch disco 6 /proyecto/src/config.txt
./mi_escribir disco /proyecto/src/config.txt "version=1.0" 0
./mi_link disco /proyecto/src/config.txt /proyecto/docs/config_link.txt

# Ambos muestran lo mismo
./mi_cat disco /proyecto/src/config.txt
./mi_cat disco /proyecto/docs/config_link.txt

# Verifica que comparten inodo
./mi_stat disco /proyecto/src/config.txt
./mi_stat disco /proyecto/docs/config_link.txt
```

### Cambiar permisos

Tabla de permisos octales:

| Valor | Lectura (4) | Escritura (2) | Ejecucion (1) | Simbolo |
|-------|-------------|---------------|---------------|---------|
| 0 | No | No | No | `---` |
| 1 | No | No | Si | `--x` |
| 2 | No | Si | No | `-w-` |
| 3 | No | Si | Si | `-wx` |
| 4 | Si | No | No | `r--` |
| 5 | Si | No | Si | `r-x` |
| 6 | Si | Si | No | `rw-` |
| 7 | Si | Si | Si | `rwx` |

Ejemplo de flujo de permisos:
```bash
# Crear fichero con lectura y escritura
./mi_touch disco 6 /proyecto/secreto.txt

# Cambiar a solo lectura
./mi_chmod disco 4 /proyecto/secreto.txt

# Intentar escribir fallara
./mi_escribir disco /proyecto/secreto.txt "datos" 0
# Error: No hay permisos de escritura

# Restaurar escritura
./mi_chmod disco 6 /proyecto/secreto.txt
```

---

## 6. Errores Frecuentes

### "Camino incorrecto"

**Causa:** La ruta no empieza con `/`. Todas las rutas deben ser absolutas.

**Solucion:**
```bash
# Incorrecto
./mi_mkdir disco 7 docs/

# Correcto
./mi_mkdir disco 7 /docs/
```

### "Es un fichero, no debe acabar en '/'"

**Causa:** Usaste `mi_touch` con una ruta que termina en `/`. Los ficheros no llevan `/` final.

**Solucion:**
```bash
# Incorrecto
./mi_touch disco 6 /docs/notas.txt/

# Correcto
./mi_touch disco 6 /docs/notas.txt
```

### "El directorio no esta vacio"

**Causa:** Intentaste usar `mi_rmdir` en un directorio que contiene ficheros o subdirectorios.

**Solucion:** Elimina primero todo el contenido:
```bash
./mi_rm disco /docs/fichero.txt
./mi_rmdir disco /docs/
```

### "No existe algun directorio intermedio"

**Causa:** Intentaste crear un fichero o directorio en una ruta donde algun directorio padre no existe.

**Solucion:** Crea los directorios intermedios primero:
```bash
# Esto falla si /docs/ no existe
./mi_touch disco 6 /docs/trabajo/notas.txt

# Primero crea el padre
./mi_mkdir disco 7 /docs/
./mi_touch disco 6 /docs/trabajo/notas.txt
```

### "No hay permisos de escritura"

**Causa:** El fichero o directorio no tiene permiso de escritura activado.

**Solucion:** Cambia los permisos antes de escribir:
```bash
./mi_chmod disco 6 /docs/notas.txt
./mi_escribir disco /docs/notas.txt "nuevo contenido" 0
```

### "No se puede borrar el directorio raiz"

**Causa:** Intentaste eliminar `/` con `mi_rm`. El directorio raiz esta protegido.

**Solucion:** No intentes borrar la raiz. Elimina solo los ficheros y subdirectorios dentro de ella.

### "Error: modo invalido"

**Causa:** Los permisos indicados no estan en el rango 0-7.

**Solucion:** Usa un valor octal valido:
```bash
# Incorrecto
./mi_mkdir disco 8 /docs/

# Correcto
./mi_mkdir disco 7 /docs/
```

---

## 7. Consejos de Uso

- **Rutas absolutas siempre.** Todas las rutas deben empezar con `/`. No hay concepto de directorio de trabajo actual.

- **Directorios con `/` final, ficheros sin ella.** Esta es la regla mas importante. `mi_mkdir` y `mi_rmdir` exigen `/` al final. `mi_touch`, `mi_escribir`, `mi_cat` y `mi_link` lo rechazan.

- **Permisos en octal 0-7.** No uses valores como 644 o 755. El sistema usa un solo digito octal: 4 para lectura, 2 para escritura, 1 para ejecucion. Sumalos para combinar.

- **Diagnostica con `leer_sf`.** Si algo va mal, ejecuta `./leer_sf disco` para ver cuantos bloques e inodos libres quedan.

- **Limpia antes de recompilar.** Ejecuta `make clean` antes de cambiar de nivel de compilacion para evitar objetos hu erfanos.

- **Usa `mi_stat` para depurar.** Si un fichero no se comporta como esperas, `mi_stat` te muestra su inodo, permisos, tamaño y numero de enlaces.

- **Los enlaces fisicos comparten inodo.** Si creas un enlace con `mi_link`, ambos caminos apuntan al mismo inodo. Cambiar permisos en uno afecta al otro.

- **`mi_rm` con enlaces fisicos.** Si un fichero tiene varios enlaces, `mi_rm` solo elimina la entrada del directorio. Los datos persisten hasta que `nlinks` llegue a 0.

---

## 8. Compilacion

### Compilar todo (nivel maximo)

```bash
make
```

Compila todos los programas disponibles (niveles 1 a 13).

### Compilar hasta un nivel especifico

```bash
make LEVEL=6
```

Compila solo los programas de los niveles 1 a 6. Util para trabajar de forma incremental.

### Niveles de compilacion

| Nivel | Funcionalidad | Programas incluidos |
|-------|--------------|---------------------|
| 1-3 | Bases: superbloque, mapa de bits, inodos | `mi_mkfs`, `leer_sf` |
| 4-6 | Ficheros y punteros (directos e indirectos) | `escribir`, `leer`, `truncar`, `permitir` |
| 7-10 | Directorios, path I/O, enlaces | `mi_mkdir`, `mi_touch`, `mi_chmod`, `mi_ls`, `mi_link`, `mi_escribir`, `mi_cat`, `mi_stat`, `mi_rm`, `mi_rmdir`, `prueba_nivel7` |
| 11-13 | Semaforos, concurrencia, simulacion | `simulacion`, `verificacion` |

### Limpiar archivos de compilacion

```bash
make clean
```

Elimina todos los archivos `.o`, los ejecutables y cualquier disco virtual (`disco*`, `ext*`).

### Flags de depuracion

Los flags `DEBUGN3` a `DEBUGN8` en el Makefile controlan trazas de depuracion. Para activar o desactivar:

```makefile
CFLAGS=-c -g -Wall -std=gnu99 -DDEBUGN3=1 -DDEBUGN4=1 -DDEBUGN5=1 -DDEBUGN6=1 -DDEBUGN7=0 -DDEBUGN8=0
```

Cambia el valor a `1` para activar las trazas de un nivel o a `0` para silenciarlas.

### Ejecutar todos los tests

```bash
make test
```

Ejecuta la bateria completa de scripts de prueba para todos los niveles.

---

## Autores

- Calvache Vidal, Antonio
- Mercadal Forteza, Lluis
- Salvatierra Romero, Diego Jeroni
