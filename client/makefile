APPNAME=client
LIBPATH=./lib/
INCPATH=./include/

CFLAGS+=-I${INCPATH}
CFLAGS+=-I ./openlibs/sqlite/install/include
CFLAGS+=-I ./openlibs/cJSON/install/include/cjson/
CFLAGS+=-I ./openlibs/libevent/install/include/ 
CFLAGS+=-I ./openlibs/openssl/install/include
LDFLAGS+=-L${LIBPATH}
LDFLAGS+=-L ./openlibs/sqlite/install/lib 
LDFLAGS+=-L ./openlibs/cJSON/install/lib/
LDFLAGS+=-L ./openlibs/libevent/install/lib/
LDFLAGS+=-L ./openlibs/openssl/install/lib

CC=gcc

all:
	${CC} ${CFLAGS} -g main.c -o ${APPNAME} ${LDFLAGS} -lmyclient -lsqlite3 -lcjson -lssl -lcrypto -levent_openssl -levent

clean:
	rm -f ${APPNAME}

run:
	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${LIBPATH} && ./client -i 127.0.0.1 -p 11111 -t 5 -d
