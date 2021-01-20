obj-m += vring_stat.o

KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf *.ko *.mod* *.o modules.* Module.symvers

install:
	insmod vring_stat.ko

remove:
	rmmod vring_stat
