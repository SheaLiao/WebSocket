APPNAME=websocket

LIBPATH=./lib/
INCPATH=./include/


CFLAGS+=-I${INCPATH}
CFLAGS+=-I ./openlibs/cJSON/install/include/ 
CFLAGS+=-I ./openlibs/libevent/install/include/
CFLAGS+=-I ./openlibs/libgpiod/install/include/
LDFLAGS+=-L${LIBPATH}
LDFLAGS+=-L ./openlibs/cJSON/install/lib/
LDFLAGS+=-L ./openlibs/libevent/install/lib/
LDFLAGS+=-L ./openlibs/libgpiod/install/lib/


CC=gcc

all:
	make -C openlibs/cJSON
	make -C openlibs/libevent
	make -C openlibs/libgpiod
	make -C src
	${CC} ${CFLAGS} -g websocket.c -o ${APPNAME} ${LDFLAGS} -lmywslib -levent -lcjson -lgpiod -pthread

clean:
	make clean -C openlibs/cJSON
	make clean -C openlibs/libevent
	make clean -C openlibs/libgpiod
	make clean -C src
	rm -f ${APPNAME}

run:
	sudo -E LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${LIBPATH}  ./websocket
