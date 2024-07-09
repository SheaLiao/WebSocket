APPNAME=websocket
LIBPATH=./lib/
INCPATH=./include/


CFLAGS+=-I${INCPATH}
CFLAGS+=-I ./openlibs/libevent/install/include/ 
CFLAGS+=-I ./openlibs/cJSON/install/include
LDFLAGS+=-L${LIBPATH}
LDFLAGS+=-L ./openlibs/libevent/install/lib/
LDFLAGS+=-L ./openlibs/cJSON/install/lib/

CC=gcc

all:
	${CC} ${CFLAGS} -g websocket.c -o ${APPNAME} ${LDFLAGS} -lmywslib  -levent -lcjson

clean:
	rm -f ${APPNAME}

run:
	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${LIBPATH} && ./websocket
