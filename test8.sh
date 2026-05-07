# Test de casos limite de directorio (Nivel 8)
set -e

clear
make clean
make LEVEL=8

echo "=== Inicializando disco ==="
./mi_mkfs disco 100000

echo "=== Creando /dir1/ ==="
./mi_mkdir disco 7 /dir1/

echo "=== Test: crear directorio existente (debe fallar) ==="
if ./mi_mkdir disco 7 /dir1/; then
    echo "FALLO: mi_mkdir deberia fallar sobre path existente"
    exit 1
fi

echo "=== Creando fichero dentro de /dir1/ ==="
./mi_touch disco 6 /dir1/file1

echo "=== Test: borrar directorio no vacio (debe fallar) ==="
if ./mi_rm disco /dir1/; then
    echo "FALLO: mi_rm deberia fallar sobre directorio no vacio"
    exit 1
fi

echo "=== Test: mi_cat sobre directorio (debe fallar) ==="
if ./mi_cat disco /dir1/ 2>/dev/null; then
    echo "FALLO: mi_cat deberia fallar sobre directorio"
    exit 1
fi

echo "=== Test: path profundo (8 niveles) ==="
./mi_mkdir disco 7 /a/
./mi_mkdir disco 7 /a/b/
./mi_mkdir disco 7 /a/b/c/
./mi_mkdir disco 7 /a/b/c/d/
./mi_mkdir disco 7 /a/b/c/d/e/
./mi_mkdir disco 7 /a/b/c/d/e/f/
./mi_mkdir disco 7 /a/b/c/d/e/f/g/
./mi_mkdir disco 7 /a/b/c/d/e/f/g/h/
./mi_ls disco /a/b/c/d/e/f/g/h/

echo "=== Test: nombre de 60 caracteres (limite TAMNOMBRE) ==="
./mi_touch disco 6 /dir1/123456789012345678901234567890123456789012345678901234567890

echo "=== Test: listar raiz ==="
./mi_ls disco /

echo "OK: test8.sh completado"
