# Test de permisos (6 casos objetivo)
set -e

clear
make clean
make LEVEL=8

echo "=== Inicializando disco ==="
./mi_mkfs disco 100000
./mi_touch disco 6 /perm.txt
./mi_escribir disco /perm.txt "hello" 0

echo "=== Caso 1: rw- (6) -> lectura OK, escritura OK ==="
./mi_cat disco /perm.txt > /tmp/perm1.txt
[ "$(cat /tmp/perm1.txt)" = "hello" ] || { echo "FALLO: lectura con perm=6"; exit 1; }
./mi_escribir disco /perm.txt "world" 0

echo "=== Caso 2: r-- (4) -> lectura OK, escritura FALLO ==="
./mi_chmod disco 4 /perm.txt
./mi_cat disco /perm.txt > /tmp/perm2.txt
[ "$(cat /tmp/perm2.txt)" = "world" ] || { echo "FALLO: lectura con perm=4"; exit 1; }
if ./mi_escribir disco /perm.txt "fail" 0 2>/dev/null; then
    echo "FALLO: escritura deberia fallar con perm=4"
    exit 1
fi

echo "=== Caso 3: -w- (2) -> lectura FALLO, escritura OK ==="
./mi_chmod disco 2 /perm.txt
if ./mi_cat disco /perm.txt 2>/dev/null; then
    echo "FALLO: lectura deberia fallar con perm=2"
    exit 1
fi
./mi_escribir disco /perm.txt "writeok" 0

echo "=== Caso 4: --- (0) -> lectura FALLO, escritura FALLO ==="
./mi_chmod disco 0 /perm.txt
if ./mi_cat disco /perm.txt 2>/dev/null; then
    echo "FALLO: lectura deberia fallar con perm=0"
    exit 1
fi
if ./mi_escribir disco /perm.txt "fail" 0 2>/dev/null; then
    echo "FALLO: escritura deberia fallar con perm=0"
    exit 1
fi

echo "=== Caso 5: rwx (7) -> lectura OK, escritura OK ==="
./mi_chmod disco 7 /perm.txt
./mi_cat disco /perm.txt > /tmp/perm5.txt
./mi_escribir disco /perm.txt "seven" 0

echo "=== Caso 6: cambio de permisos en medio de sesion ==="
./mi_chmod disco 4 /perm.txt
./mi_cat disco /perm.txt > /tmp/perm6.txt
[ "$(cat /tmp/perm6.txt | head -c 5)" = "seven" ] || { echo "FALLO: cambio de permisos en sesion"; exit 1; }

echo "OK: test_permissions.sh completado"
