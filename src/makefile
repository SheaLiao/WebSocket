LIBNAME=mywslib
INSTPATH=`pwd`/../lib/ 
INCPATH=`pwd`/../include/ 

CFLAGS+=-I${INCPATH}

CC=gcc
AR=ar 

all: dynamic_lib static_lib
	@make install
	@make clear

dynamic_lib:
	${CC} -shared -fPIC *.c -o lib${LIBNAME}.so ${CFLAGS}

static_lib:
	${CC} -c *.c ${CFLAGS}
	${AR} -rcs lib${LIBNAME}.a *.o

install:
	cp -rf lib${LIBNAME}.* ${INSTPATH}

uninstall:
	rm -rf `pwd`/../lib/lib${LIBNAME}.*

clear:
	rm -f *.o
	rm -f lib${LIBNAME}.*
