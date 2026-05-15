#!/bin/bash
# =============================================================================
# test6c.sh — Test de Nivel 6: Truncado total de texto de varios bloques
#
# Verifica: escritura de un texto largo que ocupa varios bloques y truncado
# completo a 0 bytes, comprobando la liberación de todos los recursos.
# Uso: ./test6c.sh
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
echo -e "\x1B[38;2;17;245;120m#mostramos solo el SB\x1b[0m"
./leer_sf disco
echo

# --- Test: escritura de texto largo ---
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./escribir disco "$(cat texto2.txt)" 0\x1b[0m"
./escribir disco "$(cat texto2.txt)" 0
echo

# --- Test: truncado total a 0 bytes ---
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ time ./truncar disco 1 0\x1b[0m"
time ./truncar disco 1 0