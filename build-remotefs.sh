#!/bin/bash

src='widget/remotefs.c widget/regex.c widget/regtools.c widget/pathdup.c widget/ipv6scan.c'
def='-DSTANDALONE'
inc='-I. -Iwidget'

if test "$1" = "-d" ; then
    opt='-O0'
    echo 'opt -O0'
else
    opt='-O2'
fi
warn='-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter'


echo 'building REMOTEFS.EXE'
/usr/bin/x86_64-w64-mingw32-gcc -o REMOTEFS.EXE  $warn $opt     $def $inc $src -lws2_32  || { echo error1 ; exit 1 ; } 

echo 'building remotefs'
gcc                             -o remotefs      $warn $opt -g3 $def $inc $src           || { echo error2 ; exit 1 ; } 
gcc  -DREMOTEFS_DOTEST          -o remotefs-test $warn $opt -g3 $def $inc $src           || { echo error2 ; exit 1 ; } 

