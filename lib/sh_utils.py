from cooledit import *

def generic(s, d):
    k = current ()
    for c in s:
	if c == '\t':
	    indent (1)
	elif c == '\b':
	    indent (0)
	else:
	    insert (c)
    move_to (k + d)
    redraw_page ()

def case ():
    k = current ()
    insert ('case "" in\n')
    indent (1)
    insert ('*)\n')
    indent (1)
    insert (';;\n')
    indent (-2)
    insert ('esac\n')
    move_to (k + 6)
    redraw_page ()

