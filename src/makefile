LIBNAME=mywslib
INSTPATH=`pwd`/../lib/ 
INCPATH=`pwd`/../include/ 
EVPATH=`pwd`/../openlibs/libevent/install/include
JSONPATH=`pwd`/../openlibs/cJSON/install/include/


CFLAGS+=-I${INCPATH}
CFLAGS+=-I${EVPATH}
CFLAGS+=-I${JSONPATH}

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
	cd `pwd`/../ && mkdir lib
	cp -rf lib${LIBNAME}.* ${INSTPATH}

uninstall:
	rm -rf `pwd`/../lib/lib${LIBNAME}.*

clear:
	rm -f *.o
	rm -f lib${LIBNAME}.*
