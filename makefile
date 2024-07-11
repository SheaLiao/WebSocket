APPNAME=websocket

LIBPATH=./lib/
INCPATH=./include/


CFLAGS+=-I${INCPATH}
CFLAGS+=-I ./openlibs/cJSON/install/include/ 
CFLAGS+=-I ./openlibs/libevent/install/include/
LDFLAGS+=-L${LIBPATH}
LDFLAGS+=-L ./openlibs/cJSON/install/lib/
LDFLAGS+=-L ./openlibs/libevent/install/lib/


CC=gcc

all:
	${CC} ${CFLAGS} -g websocket.c -o ${APPNAME} ${LDFLAGS} -lmywslib -levent -lcjson -lgpiod

clean:
	rm -f ${APPNAME}

run:
	sudo LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${LIBPATH}  ./websocket
