CC=gcc
CFLAGS=-Wall -Wextra -O2

%.o:%.c
	$(CC) -c -o $@ $< $(FLAGS)

user.bin: user.o
	$(CC) -o $@ $^ $(FLAGS)

ioctl.bin: ioctl.o
	$(CC) -o $@ $^ $(FLAGS)

clean:
	rm -f *.o *~ *.bin
