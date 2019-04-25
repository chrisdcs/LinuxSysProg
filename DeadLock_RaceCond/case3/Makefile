obj-m := driver_case3.o

KERNEL_DIR = /usr/src/linux-headers-$(shell uname -r)

all:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
	gcc -o case3 case3.c -lpthread
	

clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order *~
	rm -rf case3
	
