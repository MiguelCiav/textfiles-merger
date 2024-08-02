all: pf1

pf1: pf1.o
	gcc -o pf1 pf1.o

pf1.o: pf1.c
	gcc -c pf1.c -o pf1.o

clean:
	rm -rf pf1.o pf1