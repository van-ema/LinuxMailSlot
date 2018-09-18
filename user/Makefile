CC=gcc
CFLAGS=-Wall -Wextra -O2
OBJ=user.o

%.o:%.c
	$(CC) -c -o $@ $< $(FLAGS)

user.bin: $(OBJ)
	$(CC) -o $@ $^ $(FLAGS)

clean:
	rm -f *.o *~
