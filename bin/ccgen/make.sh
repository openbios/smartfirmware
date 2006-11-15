INCLUDES="-Ic:/borlandc/include -Ic;/tjm/src/lib/tjlib -I."
LIBOPTS="-Lc:/borlandc/lib -Lc:/tjm/src/lib/tjlib"
OBJS="ccgen.obj"
LIBS="tjlib.lib"
CFLAGS="-mh -r"

bcc -P -c $CFLAGS $INCLUDES ccgen.c
bcc -eccgen.exe $CFLAGS $LIBOPTS $OBJS$ $LIBS
