CC=gcc
CFLAGS=-Wall -Wextra -O2

all: benchmark.bin create.bin

benchmark.o:benchmark.c
	$(CC) -c -o $@ $< $(FLAGS) -lpthread

create.o:create.c
	$(CC) -c -o $@ $< $(FLAGS)

benchmark.bin:benchmark.o
	$(CC) -o $@ $^ $(FLAGS) -lpthread

create.bin:create.o
	$(CC) -o $@ $^ $(FLAGS)

clean:
	rm -f *.o *~ *.bin
