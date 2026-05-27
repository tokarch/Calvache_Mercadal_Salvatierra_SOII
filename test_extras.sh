#!/bin/bash
# =============================================================================
# test_extras.sh — Prueba de integración para mi_cp_f, mi_rn, mi_mv y mi_rm_r
#
# Verifica que las funciones extra de copia, renombrado, movimiento y borrado
# recursivo funcionen correctamente en el sistema de ficheros virtual.
# Uso: ./test_extras.sh
# =============================================================================

set -e
clear

echo -e "\x1B[38;2;17;245;120m=== COMPILANDO PROYECTO ===\x1b[0m"
make clean
make

echo -e "\n\x1B[38;2;17;245;120m=== 0. PREPARANDO EL DISCO Y ENTORNO ===\x1b[0m"
./mi_mkfs disco 100000
./mi_mkdir disco 7 /pruebas/
./mi_mkdir disco 7 /pruebas/origen/
./mi_mkdir disco 7 /pruebas/destino/
./mi_touch disco 6 /pruebas/origen/documento.txt
./mi_escribir disco /pruebas/origen/documento.txt "Texto secreto para copiar y mover." 0

echo -e "\n\x1B[38;2;17;245;120m=== 1. PRUEBA: COPIAR FICHERO (mi_cp_f) ===\x1b[0m"
echo "$ ./mi_cp_f disco /pruebas/origen/documento.txt /pruebas/destino/"
./mi_cp_f disco /pruebas/origen/documento.txt /pruebas/destino/
echo "Contenido del archivo copiado en el destino:"
./mi_cat disco /pruebas/destino/documento.txt

echo -e "\n\n\x1B[38;2;17;245;120m=== 2. PRUEBA: RENOMBRAR (mi_rn) ===\x1b[0m"
echo "$ ./mi_rn disco /pruebas/destino/documento.txt doc_renombrado.txt"
./mi_rn disco /pruebas/destino/documento.txt doc_renombrado.txt
echo "Listando carpeta destino (el nombre debe haber cambiado):"
./mi_ls -l disco /pruebas/destino/

echo -e "\n\x1B[38;2;17;245;120m=== 3. PRUEBA: MOVER (mi_mv) ===\x1b[0m"
echo "Creamos un archivo nuevo para moverlo..."
./mi_touch disco 6 /pruebas/origen/archivo_volador.txt
echo "$ ./mi_mv disco /pruebas/origen/archivo_volador.txt /pruebas/destino/"
./mi_mv disco /pruebas/origen/archivo_volador.txt /pruebas/destino/
echo "Listando carpeta ORIGEN (ya NO debe estar ahí):"
./mi_ls disco /pruebas/origen/
echo "Listando carpeta DESTINO (ahora debe estar aquí):"
./mi_ls disco /pruebas/destino/

echo -e "\n\x1B[38;2;17;245;120m=== 4. PRUEBA: BORRADO RECURSIVO (mi_rm_r) ===\x1b[0m"
echo "Vamos a borrar toda la carpeta /pruebas/destino/ y su contenido."
echo "$ ./mi_rm_r disco /pruebas/destino/"
./mi_rm_r disco /pruebas/destino/
echo "Listando la carpeta principal /pruebas/:"
./mi_ls disco /pruebas/

echo -e "\n\x1B[38;2;17;245;120m=== 5. ESTADO FINAL DEL DISCO ===\x1b[0m"
./leer_sf disco

echo -e "\n\x1B[38;2;17;245;120m¡TODAS LAS FUNCIONES EXTRA SE EJECUTARON CON ÉXITO!\x1b[0m"