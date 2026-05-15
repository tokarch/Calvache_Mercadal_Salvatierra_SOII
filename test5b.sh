#!/bin/bash
# =============================================================================
# test5b.sh — Test de Nivel 5: Escritura con texto largo y verificación de permisos
#
# Verifica: escritura de texto largo en múltiples inodos, lectura a fichero
# externo, y denegación de acceso al cambiar permisos a 0.
# Uso: ./test5b.sh
# =============================================================================

set -e
clear
make clean
make LEVEL=5

# --- Inicialización del sistema de ficheros ---
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./mi_mkfs disco 100000\x1b[0m"
./mi_mkfs disco 100000
echo

# --- Test: escritura de texto largo en inodos diferentes ---
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./escribir disco "$(cat texto2.txt)" 1\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#escribimos el texto contenido en text2.txt en los offsets  9000, 209000, 30725000, \x1b[0m"
echo -e "\x1B[38;2;17;245;120m#409605000 y 480000000 de inodos diferentes\x1b[0m"
./escribir disco "$(cat texto2.txt)" 1
echo
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./leer disco 2 > ext4.txt\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#leemos el contenido del inodo 2 (escrito en el offset 209000) y lo direccionamos\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#al fichero externo ext4.txt\x1b[0m"
./leer disco 2 > ext4.txt
echo
echo -e "\x1B[38;2;17;245;120m$ ls -l ext4.txt\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#comprobamos cuánto ocupa el fichero externo ext4.txt\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#(ha de coincidir con el tamaño en bytes lógico del inodo 2 y con total_leidos)\x1b[0m"
ls -l ext4.txt
echo
echo -e "\x1B[38;2;17;245;120m$ cat ext4.txt\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#usamos el cat del sistema para leer el contenido de nuestro fichero direccionado\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#No hay que mostrar basura\x1b[0m"
cat ext4.txt
echo
echo

# --- Test: denegación de lectura con permisos 0 ---
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./permitir\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#mostramos sintaxis de permitir\x1b[0m"
./permitir || true
echo
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./permitir disco 2 0\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#cambiamos permisos del inodo 2 a 0\x1b[0m"
./permitir disco 2 0
echo
echo -e "\x1B[38;2;17;245;120m$ ./leer disco 2\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#intentamos leer inodo 2 con permisos=0\x1b[0m"
if ./leer disco 2; then
    echo "FALLO: leer deberia haber sido denegado con perm=0"
    exit 1
fi

# --- Validación final ---
[ -s ext4.txt ] || { echo "FALLO: ext4.txt esta vacio o no existe"; exit 1; }
echo "OK: test5b.sh completado"