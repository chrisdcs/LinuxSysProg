obj-m := driver_case4.o

KERNEL_DIR = /usr/src/linux-headers-$(shell uname -r)

all:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
	gcc -o case4 case4.c -lpthread
	

clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order *~
	rm -rf case4
	
