obj-m := driver_case2.o

KERNEL_DIR = /usr/src/linux-headers-$(shell uname -r)

all:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
	gcc -o case2 case2.c -lpthread
	

clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order *~
	rm -rf case2
	
