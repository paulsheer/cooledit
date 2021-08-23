touch README
touch ChangeLog
libtoolize --copy -f
aclocal --force
autoconf --force
automake --copy --add-missing --force-missing

