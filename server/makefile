APPNAME=server
LIBPATH=./lib/
INCPATH=./include/

CFLAGS+=-I${INCPATH}
CFLAGS+=-I ./openlibs/sqlite/install/include
CFLAGS+=-I ./openlibs/libevent/install/include/ 
LDFLAGS+=-L${LIBPATH}
LDFLAGS+=-L ./openlibs/sqlite/install/lib
LDFLAGS+=-L ./openlibs/libevent/install/lib/

CC=gcc

all:
	${CC} ${CFLAGS} -g main.c -o ${APPNAME} ${LDFLAGS} -lmyserver -lsqlite3 -levent

clean:
	rm -f ${APPNAME}

run:
	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${LIBPATH} && ./server -p 11111 -d
