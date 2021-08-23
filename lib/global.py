
# This is the top level Python code for Cooledit.

def type_change(s):
    menu ("Util")	# clear the Util menu
    if s == "C/C++ Program":
	import c_utils
	# make it global
	globals()["c_utils"] = c_utils
	menu ("Util", "for(;;) {", "c_utils.generic('for (;;) {', 5)")
	menu ("Util", "while() {", "c_utils.generic('while () {', 7)")
	menu ("Util", "do {", "c_utils.do_while()")
	menu ("Util", "switch() {", "c_utils.generic('switch () {', 8)")
	menu ("Util", "case:", "c_utils.case()")
	menu ("Util", "if() {", "c_utils.generic('if () {', 4)")
	menu ("Util", "main() {", "c_utils.main()")
	menu ("Util", "#include ", "c_utils.include()")
	menu ("Util", "printf();", "c_utils.printf()")

    if s == "Shell Script":
	import sh_utils
	# make it global
	globals()["sh_utils"] = sh_utils
	menu ("Util", "for", "sh_utils.generic ('for i in * ; do\\n\\t\\n\\bdone\\n' % (), 9)")
	menu ("Util", "while", "sh_utils.generic('while true ; do\\n\\t\\n\\bdone\\n' % (), 10)")
	menu ("Util", "until", "sh_utils.generic('until true ; do\\n\\t\\n\\bdone\\n' % (), 10)")
	menu ("Util", "case", "sh_utils.case ()")
	menu ("Util", "function", "sh_utils.generic('function foo ()\\n' % (), 12)")
	menu ("Util", "if", "sh_utils.generic('if test \\\"\\\" = \\\"\\\" ; then\\n\\t\\n\\bfi\\n' % (), 9)")
	menu ("Util", "if... else... elif", "sh_utils.generic('if test \\\"\\\" = \\\"\\\" ; then\\n\\t\\n\\belif test \\\"\\\" = \\\"\\\" ; then\\n\\t\\n\\belse\\n\\t\\n\\bfi\\n' % (), 9)")

