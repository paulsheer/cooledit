#!/bin/bash

src='widget/remotefs.c widget/regtools.c widget/pathdup.c widget/ipv6scan.c widget/aes.c widget/sha256.c widget/symauth.c widget/hashtable.c widget/cterminal.c widget/childhandler.c widget/mswinchild.c widget/xwinfwd.c widget/fnmatch.c'
def='-DSTANDALONE -DNO_INSPECT'
inc='-I. -Iwidget'

if test "$1" = "-d" ; then
    opt='-O0 -ggdb'
    echo
    echo '============================================================='
    echo "  $opt"
    echo '============================================================='
    echo
else
    opt='-O2 -s'
fi
warn='-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter'


echo '=================================================='
echo 'building remotefs-test'
# gcc                     -static -o remotefs      $warn $opt $def $inc $src           || { echo error2 ; exit 1 ; } 
gcc  -DREMOTEFS_DOTEST  -o remotefs-test $warn $opt $def $inc $src           || { echo error2 ; exit 1 ; } 

echo '=================================================='
echo 'building winrand.obj'
/usr/bin/x86_64-w64-mingw32-gcc -Wall -c -o winrand.obj -I/usr/share/mingw-w64/include/ widget/winrand.c || { echo error3 ; exit 1 ; }

echo '=================================================='
echo 'building remotefs_res.obj'
/usr/bin/x86_64-w64-mingw32-windres remotefs.rc -o remotefs_res.obj || { echo error4 ; exit 1 ; }

echo '=================================================='
echo 'building REMOTEFS.EXE'
/usr/bin/x86_64-w64-mingw32-gcc -o REMOTEFS.EXE  $warn $opt     $def $inc $src winrand.obj remotefs_res.obj remotefs/libbusybox.a -lws2_32 -lgdi32 -lbcrypt -lsecur32   || { echo error1 ; exit 1 ; }

