CC 		 = gcc
CPP    = g++ -std=c++11
CFLAGS = -g -Wall -Wno-unused-function 

all: test cache cache-ref

test: catch.o cache.o test.cpp
	$(CPP) $(CFLAGS) -o test catch.o cache.o test.cpp

cache: catch.o cache.o main.c
	$(CC) $(CFLAGS) -o cache cache.o main.c

cache-ref: catch.o cache-ref.o main.c
	$(CC) $(CFLAGS) -o cache-ref cache-ref.o main.c

catch.o: catch.cpp catch.hpp
	$(CPP) $(CFLAGS) -o catch.o -c catch.cpp

cache.o: cache.h cache.c
	$(CC) $(CFLAGS) -o cache.o -c cache.c

clean:
	rm -f test cache cache-ref cache.o

tidy:
	rm -f test cache cache-ref cache.o catch.o
