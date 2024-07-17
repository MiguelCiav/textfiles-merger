#
# Minishell
#

# Regla por defecto: compilar el proyecto completo
all: pf1

# Regla para limpiar los archivos generados por la compilación
clean:
	rm -rf pf1.o pf1

# Regla para compilar el ejecutable
pf1: pf1.o
	gcc -o pf1 pf1.o

# Regla para compilar el archivo minishell.c en minishell.o
pf1.o: pf1.c
	gcc -c pf1.c -o pf1.o
