
all: compare array-max

compare: compare.c compare-main.c
	gcc -g compare.c compare-main.c -o compare

array-max: array-max.s array-max-main.c
	gcc -g array-max.s array-max-main.c -o array-max

clean:
	rm -f compare array-max
	rm -f *.o

a3.zip: compare.s compare-main.s array-max.s array-max-main.c compare-main.c 
	zip $@ $?

