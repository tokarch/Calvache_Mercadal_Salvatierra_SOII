#!/bin/bash
# =============================================================================
# test_transitions.sh — Test de transiciones de bloques (boundaries)
#
# Verifica: escritura y lectura en los offsets límite entre bloques directos
# e indirectos, comprobando la correcta transición de punteros en el inodo.
# Uso: ./test_transitions.sh
# =============================================================================

set -e

clear
make clean
make LEVEL=8

# --- Inicialización del sistema de ficheros ---
echo "=== Inicializando disco ==="
./mi_mkfs disco 100000

echo "=== Creando fichero para test de boundaries ==="
./mi_touch disco 6 /boundary.txt

# --- Test: escritura en boundaries entre bloques directos e indirectos ---
echo "=== Test: escribir en offset 12287 (ultimo byte bloque directo 11) ==="
./mi_escribir disco /boundary.txt "A" 12287

echo "=== Test: escribir en offset 12288 (primer byte indirecto 0, bloque 12) ==="
./mi_escribir disco /boundary.txt "B" 12288

echo "=== Test: escribir en offset 274431 (ultimo byte indirecto 0, bloque 267) ==="
./mi_escribir disco /boundary.txt "C" 274431

echo "=== Test: escribir en offset 274432 (primer byte indirecto 1, bloque 268) ==="
./mi_escribir disco /boundary.txt "D" 274432

# --- Test: verificación de contenido en los boundaries ---
echo "=== Leyendo y verificando contenido ==="
./mi_cat disco /boundary.txt > /tmp/boundary_out.txt
[ "$(dd if=/tmp/boundary_out.txt bs=1 skip=12287 count=1 2>/dev/null)" = "A" ] || { echo "FALLO: byte en offset 12287"; exit 1; }
[ "$(dd if=/tmp/boundary_out.txt bs=1 skip=12288 count=1 2>/dev/null)" = "B" ] || { echo "FALLO: byte en offset 12288"; exit 1; }
[ "$(dd if=/tmp/boundary_out.txt bs=1 skip=274431 count=1 2>/dev/null)" = "C" ] || { echo "FALLO: byte en offset 274431"; exit 1; }
[ "$(dd if=/tmp/boundary_out.txt bs=1 skip=274432 count=1 2>/dev/null)" = "D" ] || { echo "FALLO: byte en offset 274432"; exit 1; }

echo "OK: test_transitions.sh completado"
