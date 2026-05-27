# AUTORES
# =======
# Calvache Vidal, Antonio
# Mercadal Forteza, Lluís
# Salvatierra Romero, Diego Jeroni

CC=gcc
CFLAGS=-c -g -Wall -std=gnu99 -DDEBUGN3=0 -DDEBUGN4=0 -DDEBUGN5=0 -DDEBUGN6=0 -DDEBUGN7=0 -DDEBUGN8=0 -DDEBUGN9=0 -DDEBUGN10=0 -DDEBUGN11=0 -DDEBUGN12=0 -DDEBUGN13=0
LDFLAGS=-pthread

# =========================================================================
# CONFIGURACIÓN DE NIVELES
# Uso: make LEVEL=13  (Compila hasta el nivel 13)
# Si no se especifica LEVEL, compila todo por defecto (nivel 13)
# =========================================================================
LEVEL ?= 13

# -------------------------------------------------------------------------
# NIVEL 1 - 3: Bases del sistema (Metadatos y Bloques)
# Aquí incluimos los comandos base para crear el disco y leer sus estructuras.
# -------------------------------------------------------------------------
SOURCES = mi_mkfs.c bloques.c ficheros_basico.c leer_sf.c semaforo_mutex_posix.c
LIBRARIES = bloques.o ficheros_basico.o semaforo_mutex_posix.o
INCLUDES = bloques.h ficheros_basico.h semaforo_mutex_posix.h
PROGRAMS = mi_mkfs leer_sf

# -------------------------------------------------------------------------
# NIVEL 4 - 6: Ficheros y Punteros
# -------------------------------------------------------------------------
ifneq ($(filter $(LEVEL), 4 5 6 7 8 9 10 11 12 13),)
    SOURCES += ficheros.c escribir.c leer.c truncar.c permitir.c
    LIBRARIES += ficheros.o
    INCLUDES += ficheros.h
    PROGRAMS += escribir leer truncar permitir
endif

# -------------------------------------------------------------------------
# NIVEL 7 - 10: Directorios, Path I/O y Links
# -------------------------------------------------------------------------
ifneq ($(filter $(LEVEL), 7 8 9 10 11 12 13),)
    SOURCES += directorios.c prueba_nivel7.c mi_mkdir.c mi_touch.c mi_chmod.c mi_ls.c mi_link.c mi_escribir.c mi_cat.c mi_stat.c mi_rm.c mi_rmdir.c mi_escribir_varios.c mi_rm_r.c mi_rn.c mi_mv.c mi_cp_f.c
    LIBRARIES += directorios.o
    INCLUDES += directorios.h
    PROGRAMS += prueba_nivel7 mi_mkdir mi_touch mi_chmod mi_ls mi_link mi_escribir mi_cat mi_stat mi_rm mi_rmdir mi_escribir_varios mi_rm_r mi_rn mi_mv mi_cp_f
endif

# -------------------------------------------------------------------------
# NIVEL 11 - 13: Simulación y Verificación de Concurrencia
# -------------------------------------------------------------------------
ifneq ($(filter $(LEVEL), 11 12 13),)
    SOURCES += simulacion.c verificacion.c
    INCLUDES += simulacion.h verificacion.h
    PROGRAMS += simulacion verificacion
endif

# =========================================================================
# REGLAS DE COMPILACIÓN
# =========================================================================
OBJS=$(SOURCES:.c=.o)

all: $(OBJS) $(PROGRAMS)

# Regla mejorada para enlazar los programas con sus dependencias
$(PROGRAMS): %: %.o $(LIBRARIES)
	$(CC) $(LDFLAGS) $(LIBRARIES) $< -o $@ -lm

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: test clean
test:
	@echo "=== Ejecutando test5a.sh ==="
	bash test5a.sh
	@echo "OK: test5a"
	@echo "=== Ejecutando test5b.sh ==="
	bash test5b.sh
	@echo "OK: test5b"
	@echo "=== Ejecutando test6a.sh ==="
	bash test6a.sh
	@echo "OK: test6a"
	@echo "=== Ejecutando test6b.sh ==="
	bash test6b.sh
	@echo "OK: test6b"
	@echo "=== Ejecutando test6c.sh ==="
	bash test6c.sh
	@echo "OK: test6c"
	@echo "=== Ejecutando test6d.sh ==="
	bash test6d.sh
	@echo "OK: test6d"
	@echo "=== Ejecutando scripte1.sh ==="
	bash scripte1.sh
	@echo "OK: scripte1"
	@echo "=== Recompilando programas nivel 10 ==="
	$(MAKE) clean
	$(MAKE) all
	@echo "=== Ejecutando prueba_nivel7 ==="
	./mi_mkfs disco 100000
	./prueba_nivel7 disco
	@echo "OK: prueba_nivel7"
	@echo "=== Ejecutando test7.sh ==="
	bash test7.sh
	@echo "OK: test7"
	@echo "=== Ejecutando test8.sh ==="
	bash test8.sh
	@echo "OK: test8"
	@echo "=== Ejecutando test9.sh ==="
	bash test9.sh
	@echo "OK: test9"
	@echo "=== Ejecutando test10.sh ==="
	bash test10.sh
	@echo "OK: test10"
	@echo "=== Ejecutando test_transitions.sh ==="
	bash test_transitions.sh
	@echo "OK: test_transitions"
	@echo "=== Ejecutando test_permissions.sh ==="
	bash test_permissions.sh
	@echo "OK: test_permissions"
	@echo ""
	@echo "=== TODOS LOS TESTS OK ==="

clean:
	rm -rf *.o *~ $(PROGRAMS) disco* ext* res* mi_rn mi_mv mi_cp_f