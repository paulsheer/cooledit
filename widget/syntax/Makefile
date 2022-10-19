
CFLAGS = -Wall -O0 -g3  -I../.. -I.. -DUNIT_TEST

SRC     = syntax.c stringtools.c regex.c
OBJ     = $(SRC:.c=.o)


all: test

.c.o:
	gcc $(CFLAGS) -c $<

syntax.c:
	ln -sf ../syntax.c .
stringtools.c:
	ln -sf ../stringtools.c .
regex.c:
	ln -sf ../regex.c .

syntax.o: syntax.c
stringtools.o: stringtools.c
regex.o: regex.c

syntax-test: $(OBJ)
	gcc -no-pie -o syntax-test $(OBJ)

clean:
	rm -f *.o syntax-test

test: syntax-test
	./syntax-test

