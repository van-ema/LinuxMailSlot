#!/bin/bash

make clean
make
rm ./my-mailslot
sudo ./create.bin mailslot ./my-mailslot 243 0
echo "writing\n"
sudo time ./benchmark.bin ./my-mailslot write &> /dev/null
echo "reading\n"
sudo time ./benchmark.bin ./my-mailslot read &> /dev/null
