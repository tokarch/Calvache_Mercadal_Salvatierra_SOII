# AUTORES
# =======
# Calvache Vidal, Antonio
# Mercadal Forteza, Lluís
# Salvatierra Romero, Diego Jeroni


CC=gcc
CFLAGS=-c -g -Wall -std=gnu99 -DDEBUGN3=1 -DDEBUGN4=1 -DDEBUGN5=1 -DDEBUGN6=1 -DDEBUGN7=0
LDFLAGS=-pthread

# =========================================================================
# CONFIGURACIÓN DE NIVELES
# Uso: make LEVEL=7  (Compila hasta el nivel 7)
# Si no se especifica LEVEL, compila todo por defecto (ej. nivel 7)
# =========================================================================
LEVEL ?= 7

# -------------------------------------------------------------------------
# NIVEL 1 - 3: Bases del sistema (Metadatos y Bloques)
# -------------------------------------------------------------------------
SOURCES = mi_mkfs.c bloques.c ficheros_basico.c leer_sf.c
LIBRARIES = bloques.o ficheros_basico.o
INCLUDES = bloques.h ficheros_basico.h
PROGRAMS = mi_mkfs leer_sf

# -------------------------------------------------------------------------
# NIVEL 4 - 6: Ficheros y Punteros
# -------------------------------------------------------------------------
ifneq ($(filter $(LEVEL), 4 5 6 7),)
    SOURCES += ficheros.c escribir.c leer.c truncar.c permitir.c
    LIBRARIES += ficheros.o
    INCLUDES += ficheros.h
    PROGRAMS += escribir leer truncar permitir
endif

# -------------------------------------------------------------------------
# NIVEL 7: Directorios y Comandos básicos
# -------------------------------------------------------------------------
ifneq ($(filter $(LEVEL), 7),)
    SOURCES += directorios.c prueba_nivel7.c mi_mkdir.c mi_chmod.c mi_ls.c mi_link.c mi_escribir.c mi_cat.c mi_stat.c mi_rm.c
    LIBRARIES += directorios.o
    INCLUDES += directorios.h
    PROGRAMS += prueba_nivel7 mi_mkdir mi_chmod mi_ls mi_link mi_escribir mi_cat mi_stat mi_rm
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

.PHONY: clean
clean:
	rm -rf *.o *~ $(PROGRAMS) disco* ext*