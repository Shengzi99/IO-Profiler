CC=gcc
CXX=g++

CFLAGS=-O0
LFLAGS=-ldl -lrt

LIBS+=
INCLUDES+=


SRCS = ./src/libioProf.cpp

all:test

libioProf.so: ${SRCS}
	${CC} -fPIC -shared ${CFLAGS} ${INCLUDES} ${LIBS}   ${LFLAGS} -o $@ $^

test: example/test.cpp libioProf.so
	${CXX} ${CFLAGS} -std=c++11 -o $@ $<

clean:
	rm -rf  ./libioProf.so ./test

run:
	# ./test
	LD_PRELOAD=./libioProf.so ./test 
