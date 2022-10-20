
.PHONY: test clean all

CFLAGS = -Wall -O0 -g3  -I../.. -I.. -DUNIT_TEST

SRC     = syntax.c stringtools.c regex.c
OBJ     = $(SRC:.c=.o)


all: ../syntax-top.c test

../syntax-top.c:
	python3 make-syntax.py > ../syntax-top.c
syntax.c:
	ln -sf ../syntax.c .
stringtools.c:
	ln -sf ../stringtools.c .
regex.c:
	ln -sf ../regex.c .

syntax.o: syntax.c
	gcc $(CFLAGS) -c $<
syntax-colors.o: syntax.c
	gcc -DPRINT_COLORS $(CFLAGS) -c $<
stringtools.o: stringtools.c
	gcc $(CFLAGS) -c $<
regex.o: regex.c
	gcc $(CFLAGS) -c $<

syntax-test: $(OBJ)
	gcc -no-pie -o syntax-test $(OBJ)

syntax-test-colors: syntax-colors.o $(OBJ)
	gcc -no-pie -o syntax-test-colors $(OBJ)

clean:
	rm -f *.o syntax-test

test: syntax-test unit.syntax uxit.syntax Makefile
	./syntax-test

colors: syntax-test-colors unit.syntax uxit.syntax Makefile
	./syntax-test-colors | tr ' ' '\n' | grep -v '/' | grep -v '^ *$$' | sort -u > colors.txt
	for i in `cat colors.txt` ; do grep -i -q -w $$i rgb2.txt || echo $$i ; done


