APPNAME=websocket
LIBPATH=./lib/
INCPATH=./include/

CFLAGS+=-I${INCPATH}
CFLAGS+=-I ./openlibs/libevent/install/include/
LDFLAGS+=-L${LIBPATH}
LDFLAGS+=-L ./openlibs/libevent/install/lib/

CC=gcc

all:
	${CC} ${CFLAGS} -g websocket.c -o ${APPNAME} ${LDFLAGS} -lmywslib -lssl -lcrypto -levent

clean:
	rm -f ${APPNAME}

run:
	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${LIBPATH} && ./websocket
