set -e

clear
make clean
make LEVEL=10

echo "=== Inicializando disco ==="
./mi_mkfs disco 100000

echo "=== Creando directorio y fichero ==="
./mi_mkdir disco 7 /dir1/
./mi_touch disco 6 /dir1/file1
./mi_escribir disco /dir1/file1 "contenido original" 0

echo "=== Test: nlinks inicial ==="
./mi_stat disco /dir1/file1 | grep -q "nlinks: 1" || { echo "FALLO: nlinks deberia ser 1"; exit 1; }

echo "=== Test: crear enlace (mi_link) ==="
./mi_link disco /dir1/file1 /dir1/link1

echo "=== Test: nlinks despues de link ==="
./mi_stat disco /dir1/file1 | grep -q "nlinks: 2" || { echo "FALLO: nlinks deberia ser 2"; exit 1; }

echo "=== Test: leer via enlace ==="
./mi_cat disco /dir1/link1 > /tmp/test10_link.txt
[ "$(cat /tmp/test10_link.txt)" = "contenido original" ] || { echo "FALLO: contenido del enlace no coincide"; exit 1; }

echo "=== Test: escribir via enlace ==="
./mi_escribir disco /dir1/link1 "modificadoXXXXXX" 0
./mi_cat disco /dir1/file1 > /tmp/test10_modified.txt
[ "$(cat /tmp/test10_modified.txt | head -c 16)" = "modificadoXXXXXX" ] || { echo "FALLO: escritura via enlace fallo"; exit 1; }

echo "=== Test: borrar enlace (mi_rm) ==="
./mi_rm disco /dir1/link1

echo "=== Test: nlinks despues de borrar enlace ==="
./mi_stat disco /dir1/file1 | grep -q "nlinks: 1" || { echo "FALLO: nlinks deberia ser 1 tras borrar enlace"; exit 1; }

echo "=== Test: leer fichero original sigue funcionando ==="
./mi_cat disco /dir1/file1 > /tmp/test10_final.txt
[ "$(cat /tmp/test10_final.txt | head -c 16)" = "modificadoXXXXXX" ] || { echo "FALLO: fichero original corrupto"; exit 1; }

echo "=== Test: link a directorio debe fallar ==="
if ./mi_link disco /dir1/ /dir1/badlink 2>/dev/null; then
    echo "FALLO: mi_link deberia fallar sobre directorio"
    exit 1
fi

echo "=== Test: crear y borrar fichero ==="
./mi_touch disco 6 /dir1/file2
./mi_rm disco /dir1/file2
if ./mi_stat disco /dir1/file2 2>/dev/null; then
    echo "FALLO: el fichero no deberia existir tras rm"
    exit 1
fi

echo "OK: test10.sh completado"
