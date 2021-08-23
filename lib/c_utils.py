import string
from cooledit import *

def go_eol ():
    move_to (eol (current_line ()))

def count_braces ():
    i = current_line ()
    s = ""
    for j in xrange (i, 0, -1):
	s = get_line (j)
	if string.strip (s):		# find non-blank line
	    break
    j = 0
    for c in s:
	if c == "{":			# count curly braces
	    j = j + 1
    return j

def generic(s, d):
    i = count_braces ()
    go_eol ()
    insert ("\n")
    indent (i)
    k = current ()
    insert (s + "\n")
    indent (0)
    insert ("}")
    move_to (k + d)
    redraw_page ()

def case():
    i = current_line ()
    s = ""
    for j in xrange (i, 0, -1):
	s = get_line (j)
	if s:
	    if string.find (s, "switch") >= 0 or s[0] == "{":		# find switch statement
		break
    s = s[:len (s) - len (string.lstrip (s))]
    go_eol ()
    insert ("\n")
    insert (s);
    k = current ()
    insert ("case :\n")
    indent (1)
    insert ("break;")
    move_to (k + 5)
    redraw_page ()

def do_while():
    i = count_braces ()
    go_eol ()
    insert ("\n")
    indent (i)
    insert ("do {\n")
    indent (0)
    insert ("} while ()")
    move (-1)
    redraw_page ()

def main():
    go_eol ()
    insert ('\nint main (int argc, char **argv)\n{\n\n}\n')
    move (-3)
    redraw_page ()

def include():
    go_eol ()
    insert ('\n#include <>')
    move (-1)
    redraw_page ()

def printf():
    i = count_braces ()
    go_eol ()
    insert ("\n")
    indent (i)
    k = current ()
    insert ('printf ("");\n')
    move_to (k + 9)
    redraw_page ()
