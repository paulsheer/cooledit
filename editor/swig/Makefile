all: ../coolpython.c

../coolpython.c: cooledit.i
	swig -python -o $@ $<
	touch ../_coolpython.c

