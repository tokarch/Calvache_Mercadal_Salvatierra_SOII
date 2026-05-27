#!/bin/bash
# =============================================================================
# test9.sh — Test de Nivel 9: Escritura, lectura y estadísticas vía path
#
# Verifica: escritura y lectura de ficheros usando rutas (mi_escribir, mi_cat),
# escritura en offset intermedio, y consulta de estadísticas (mi_stat).
# Uso: ./test9.sh
# =============================================================================

set -e

clear
make clean
make LEVEL=10

# --- Inicialización del sistema de ficheros ---
echo "=== Inicializando disco ==="
./mi_mkfs disco 100000

echo "=== Creando directorio y fichero ==="
./mi_mkdir disco 7 /dir1/
./mi_touch disco 6 /dir1/file1

# --- Test: escritura y lectura básicas ---
echo "=== Test: escribir via path (mi_escribir) ==="
./mi_escribir disco /dir1/file1 "hola mundo" 0

echo "=== Test: leer via path (mi_cat) ==="
./mi_cat disco /dir1/file1 > /tmp/test9_out.txt
[ "$(cat /tmp/test9_out.txt)" = "hola mundo" ] || { echo "FALLO: contenido no coincide"; exit 1; }

# --- Test: escritura en offset y estadísticas ---
echo "=== Test: escribir en offset via path ==="
./mi_escribir disco /dir1/file1 "XYZ" 5
./mi_cat disco /dir1/file1 > /tmp/test9_out2.txt
[ "$(cat /tmp/test9_out2.txt)" = "hola XYZdo" ] || { echo "FALLO: escritura en offset no coincide"; exit 1; }

echo "=== Test: stat via path ==="
./mi_stat disco /dir1/file1 | grep -q "nlinks: 1" || { echo "FALLO: nlinks deberia ser 1"; exit 1; }

echo "OK: test9.sh completado"
