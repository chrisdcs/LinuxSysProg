obj-m := driver_case1.o

KERNEL_DIR = /usr/src/linux-headers-$(shell uname -r)

all:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
	gcc -o case1 case1.c -lpthread
	

clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order *~
	rm -rf case1
	
