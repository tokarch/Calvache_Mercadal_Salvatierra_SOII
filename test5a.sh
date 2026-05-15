#!/bin/bash
# =============================================================================
# test5a.sh — Test de Nivel 5: Escritura y lectura básicas en múltiples offsets
#
# Verifica: inicialización del SF, escritura de un mismo texto en offsets
# grandes de un mismo inodo y de inodos diferentes, lectura a ficheros
# externos y coincidencia de tamaños y contenidos.
# Uso: ./test5a.sh
# =============================================================================

set -e
clear
make clean
make LEVEL=5

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
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./escribir\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#consultamos sintaxis comando\x1b[0m"
./escribir || true
echo

# --- Test: escritura en un mismo inodo (múltiples offsets) ---
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./escribir disco "123456789" 0\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#escribimos el texto "123456789" en los offsets  9000, 209000, 30725000, \x1b[0m"
echo -e "\x1B[38;2;17;245;120m#409605000 y 480000000 de un mismo inodo\x1b[0m"
./escribir disco "123456789" 0
echo
echo -e "\x1B[38;2;17;245;120m$ ./leer disco 1 > ext1.txt\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#leemos el contenido del inodo 1 y lo direccionamos al fichero externo ext1.txt\x1b[0m"
echo
./leer disco 1 > ext1.txt
echo
echo -e "\x1B[38;2;17;245;120m$ ls -l ext1.txt\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#comprobamos cuánto ocupa el fichero externo\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#(ha de coincidir con el tamaño en bytes lógico del inodo y con los bytes leídos)\x1b[0m"
echo
ls -l ext1.txt
echo

# --- Test: escritura en inodos diferentes ---
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./escribir disco "123456789" 1\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#escribimos el texto "123456789" en los offsets 9000, 209000, 30725000, \x1b[0m"
echo -e "\x1B[38;2;17;245;120m#409605000 y 480000000, de inodos diferentes\x1b[0m"
./escribir disco "123456789" 1
echo
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./leer disco 2 > ext2.txt\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#leemos el contenido del inodo 2 (escrito en el offset 209000) y lo direccionamos\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#al fichero externo ext2.txt\x1b[0m"
./leer disco 2 > ext2.txt
echo
echo -e "\x1B[38;2;17;245;120m$ ls -l ext2.txt\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#comprobamos cuánto ocupa el fichero externo ext2.txt\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#(ha de coincidir con el tamaño en bytes lógico del inodo 2 y con total_leidos)\x1b[0m"
ls -l ext2.txt
echo
echo -e "\x1B[38;2;17;245;120m$ cat ext2.txt\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#usamos el comando cat del sistema para leer el contenido del fichero externo\x1b[0m"
cat ext2.txt
echo
echo
echo -e "\x1B[38;2;17;245;120m$ ./leer disco 2\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#leemos el contenido de nuestro inodo 2\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#(ha de contener lo mismo que el fichero externo ext2.txt)\x1b[0m"
./leer disco 2
echo

# --- Test: lectura de inodo con offset muy grande ---
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./leer disco 5 > ext3.txt\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#leemos todo el contenido del inodo 5 (escrito en el offset 409605000) y lo \x1b[0m"
echo -e "\x1B[38;2;17;245;120m#direccionamos al fichero externo ext3.txt\x1b[0m"
./leer disco 5 > ext3.txt
echo
echo -e "\x1B[38;2;17;245;120m$ ls -l ext3.txt\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#comprobamos cuánto ocupa el fichero externo ext3.txt\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#(ha de coincidir con el tamaño en bytes lógico del inodo 5 y con total_leidos)\x1b[0m"
ls -l ext3.txt
echo
echo -e "\x1B[38;2;17;245;120m$ cat ext3.txt\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#usamos el comando cat del sistema para leer el contenido del fichero externo\x1b[0m"
tail -c 20 ext3.txt
echo
echo
echo -e "\x1B[38;2;17;245;120m$ ./leer disco 5\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#leemos el contenido de nuestro inodo 5\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#(ha de contener lo mismo que el fichero externo ext3.txt)\x1b[0m"
./leer disco 5

# --- Validación final ---
[ -s ext1.txt ] || { echo "FALLO: ext1.txt esta vacio o no existe"; exit 1; }
[ -s ext2.txt ] || { echo "FALLO: ext2.txt esta vacio o no existe"; exit 1; }
[ -s ext3.txt ] || { echo "FALLO: ext3.txt esta vacio o no existe"; exit 1; }
echo "OK: test5a.sh completado"