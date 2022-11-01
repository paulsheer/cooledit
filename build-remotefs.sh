#!/bin/bash

src='widget/remotefs.c widget/regex.c widget/regtools.c widget/pathdup.c widget/ipv6scan.c widget/aes.c widget/sha256.c widget/symauth.c widget/cterminal.c widget/childhandler.c'
def='-DSTANDALONE -DNO_INSPECT'
inc='-I. -Iwidget'

if test "$1" = "-d" ; then
    opt='-O0 -g3 '
    echo 'opt -O0'
else
    opt='-O2 -s'
fi
warn='-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter'


echo 'building remotefs-test'
# gcc                     -static -o remotefs      $warn $opt $def $inc $src           || { echo error2 ; exit 1 ; } 
gcc  -DREMOTEFS_DOTEST  -static -o remotefs-test $warn $opt $def $inc $src           || { echo error2 ; exit 1 ; } 

echo 'building winrand.obj'
/usr/bin/x86_64-w64-mingw32-gcc -Wall -c -o winrand.obj -I/usr/share/mingw-w64/include/ widget/winrand.c || { echo error3 ; exit 1 ; } 
echo 'building REMOTEFS.EXE'
/usr/bin/x86_64-w64-mingw32-gcc -o REMOTEFS.EXE  $warn $opt     $def $inc $src winrand.obj -lws2_32  || { echo error1 ; exit 1 ; } 

