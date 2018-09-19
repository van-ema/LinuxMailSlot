#!/bin/bash

make clean
make
rm ./my-fifo
./create.bin fifo ./my-fifo
echo "writing\n"
time ./benchmark.bin ./my-fifo write &> /dev/null
echo "reading\n"
time ./benchmark.bin ./my-fifo read &> /dev/null
