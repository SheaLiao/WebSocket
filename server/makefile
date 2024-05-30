APPNAME=server
LIBPATH=./lib/
INCPATH=./include/

CFLAGS+=-I${INCPATH}
CFLAGS+=-I ./openlibs/sqlite/install/include
CFLAGS+=-I ./openlibs/libevent/install/include/ 
#CFLAGS+=-I ./openlibs/openssl/install/include
LDFLAGS+=-L${LIBPATH}
LDFLAGS+=-L ./openlibs/sqlite/install/lib 
LDFLAGS+=-L ./openlibs/libevent/install/lib/
#LDFLAGS+=-L ./openlibs/openssl/install/lib

CC=gcc

all:
	${CC} ${CFLAGS} -g main.c -o ${APPNAME} ${LDFLAGS} -lmyserver -lpthread -lsqlite3 -lssl -lcrypto -levent_openssl -levent

clean:
	rm -f ${APPNAME}

run:
	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${LIBPATH} && ./server -p 11111 -d
