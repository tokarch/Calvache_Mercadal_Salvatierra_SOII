#!/bin/bash
# =============================================================================
# scripte3.sh — Test de simulación y verificación
#
# Verifica: compilación sin trazas de debug, creación del dispositivo,
# ejecución de la simulación, verificación de resultados, generación del
# informe y lectura del superbloque final.
# Uso: ./scripte3.sh
# =============================================================================

make clean
make CFLAGS="-c -g -Wall -std=gnu99 -DDEBUGN3=0 -DDEBUGN4=0 -DDEBUGN5=0 -DDEBUGN6=0 -DDEBUGN7=0 -DDEBUGN8=0"

# --- Inicialización del dispositivo ---
./mi_mkfs disco 100000

#Obtenemos la fecha
current_date=`date +%Y%m%d%H%M%S` #yearmonthdayHourMinuteSecond

# --- Simulación de procesos ---
echo
echo -e "\x1B[38;2;17;245;120m######################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ time ./simulacion disco\x1b[0m"
time ./simulacion disco

#Modificamos el string del directorio para la verificación
sim_dir="simul_"$current_date #simul_aaaammddhhmmss

# --- Verificación de la simulación ---
echo -e "\x1B[38;2;17;245;120m######################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ time ./verificacion disco /$sim_dir/\x1b[0m"
time ./verificacion disco /$sim_dir/

# --- Generación y consulta del informe ---
echo -e "\x1B[38;2;17;245;120m######################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./mi_cat disco /$sim_dir/informe.txt > res.txt\x1b[0m"
./mi_cat disco /$sim_dir/informe.txt > res.txt

echo -e "\x1B[38;2;17;245;120m######################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ls -l res.txt\x1b[0m"
ls -l res.txt

echo -e "\x1B[38;2;17;245;120m######################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ cat res.txt\x1b[0m"
cat res.txt

# --- Estado final del superbloque ---
echo -e "\x1B[38;2;17;245;120m######################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./leer_sf disco\x1b[0m"
./leer_sf disco

make clean