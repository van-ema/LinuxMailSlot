obj-m += ms.o
ms-objs := mailslot.o fifo_wait_queue.o
MODULE += ms.ko

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 

load:
	sudo insmod ./$(MODULE)

unload:
	sudo rmmod -f ./$(MODULE)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
