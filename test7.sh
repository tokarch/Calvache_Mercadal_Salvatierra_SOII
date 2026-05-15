#!/bin/bash
# =============================================================================
# test7.sh — Test de Nivel 7-8: Directorios, ficheros, permisos y enlaces
#
# Verifica: creación de directorios y ficheros, escritura y lectura mediante
# rutas, control de permisos con mi_chmod, enlaces duros (mi_link) y borrado
# de ficheros y directorios.
# Uso: ./test7.sh
# =============================================================================

set -e

clear
make clean
make LEVEL=8

# --- Inicialización del sistema de ficheros ---
echo "=== Inicializando disco ==="
./mi_mkfs disco 100000

# --- Test: creación de directorios y ficheros ---
echo "=== Creando directorio /dir1/ ==="
./mi_mkdir disco 7 /dir1/

echo "=== Creando fichero /dir1/file1 ==="
./mi_touch disco 6 /dir1/file1

# --- Test: escritura y lectura ---
echo "=== Escribiendo en /dir1/file1 ==="
./mi_escribir disco /dir1/file1 "hello" 0

echo "=== Leyendo /dir1/file1 ==="
./mi_cat disco /dir1/file1 > /tmp/test7_out.txt
[ "$(cat /tmp/test7_out.txt)" = "hello" ] || { echo "FALLO: contenido no coincide"; exit 1; }

echo "=== Stat de /dir1/file1 ==="
./mi_stat disco /dir1/file1

# --- Test: control de permisos ---
echo "=== Cambiando permisos a 0 ==="
./mi_chmod disco 0 /dir1/file1

echo "=== Intentando leer con perm=0 (debe fallar) ==="
if ./mi_cat disco /dir1/file1 2>/dev/null; then
    echo "FALLO: mi_cat deberia haber sido denegado con perm=0"
    exit 1
fi

echo "=== Restaurando permisos ==="
./mi_chmod disco 6 /dir1/file1

# --- Test: listado de directorios ---
echo "=== Listando /dir1/ ==="
./mi_ls disco /dir1/

# --- Test: enlaces y borrado ---
echo "=== Creando enlace /dir1/link1 ==="
./mi_link disco /dir1/file1 /dir1/link1

echo "=== Listando /dir1/ con enlace ==="
./mi_ls disco /dir1/

echo "=== Borrando enlace /dir1/link1 ==="
./mi_rm disco /dir1/link1

echo "=== Borrando fichero /dir1/file1 ==="
./mi_rm disco /dir1/file1

echo "=== Listando /dir1/ vacio ==="
./mi_ls disco /dir1/

echo "OK: test7.sh completado"
