


f = open('Syntax.in').readlines()

for i in f:
    i = i.rstrip()
    i = i.replace('@PERL@', 'perl')
    i = i.replace('@PYTHON@', 'python')
    i = i.replace('@RUBY@', 'ruby')
    i = i.replace('\\', '\\\\')
    i = i.replace('\"', '\\\"')
    print('"%s",' % i)

