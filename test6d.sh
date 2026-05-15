#!/bin/bash
# =============================================================================
# test6d.sh — Test de Nivel 6: Truncados parciales de texto de varios bloques
#
# Verifica: escritura de un texto largo en múltiples offsets y truncados
# parciales progresivos comprobando la liberación de bloques de datos e indirectos.
# Uso: ./test6d.sh
# =============================================================================

set -e
clear
make clean
make LEVEL=6

# --- Inicialización del sistema de ficheros ---
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./mi_mkfs disco 100000\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#inicializamos el sistema de ficheros con 100.000 bloques\x1b[0m"
./mi_mkfs disco 100000
echo
echo -e "\x1B[38;2;17;245;120m$ ./leer_sf disco\x1b[0m"
./leer_sf disco
echo

# --- Test: escritura de texto largo ---
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./escribir disco "$(cat texto2.txt)" 0\x1b[0m"
./escribir disco "$(cat texto2.txt)" 0
echo
echo -e "\x1B[38;2;17;245;120m$ ./leer_sf disco\x1b[0m"
./leer_sf disco
echo

# --- Test: truncados parciales progresivos ---
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ time ./truncar disco 1 409605001\x1b[0m"
time ./truncar disco 1 409605001
echo
echo -e "\x1B[38;2;17;245;120m$ ./leer_sf disco\x1b[0m"
./leer_sf disco
echo
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ time ./truncar disco 1 30725003\x1b[0m"
time ./truncar disco 1 30725003
echo
echo -e "\x1B[38;2;17;245;120m$ ./leer_sf disco\x1b[0m"
./leer_sf disco
echo
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ time ./truncar disco 1 209008\x1b[0m"
time ./truncar disco 1 209008
echo
echo -e "\x1B[38;2;17;245;120m$ ./leer_sf disco\x1b[0m"
./leer_sf disco
echo
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ time ./truncar disco 1 9005\x1b[0m"
time ./truncar disco 1 9005
echo
echo -e "\x1B[38;2;17;245;120m$ ./leer_sf disco\x1b[0m"
./leer_sf disco
echo
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ time ./truncar disco 1 0\x1b[0m"
time ./truncar disco 1 0
echo
echo -e "\x1B[38;2;17;245;120m$ ./leer_sf disco\x1b[0m"
./leer_sf disco