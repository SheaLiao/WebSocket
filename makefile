APPNAME=websocket
LIBPATH=./lib/
INCPATH=./include/

CFLAGS+=-I${INCPATH}
LDFLAGS+=-L${LIBPATH}

CC=gcc

all:
	${CC} ${CFLAGS} -g websocket.c -o ${APPNAME} ${LDFLAGS} -lmywslib -lssl -lcrypto -levent -lcjson

clean:
	rm -f ${APPNAME}

run:
	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${LIBPATH} && ./websocket
