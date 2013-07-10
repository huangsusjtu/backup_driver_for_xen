obj-m := xen-blkback.o
xen-blkback-y := blkback.o xenbus.o file_log.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

install:
	rm  /lib/modules/$(shell uname -r)/kernel/drivers/block/xen-blkback/xen-blkback.ko	
	mv ./xen-blkback.ko /lib/modules/$(shell uname -r)/kernel/drivers/block/xen-blkback/

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean







#obj-$(CONFIG_XEN_BLKDEV_BACKEND) := xen-blkback.o

#xen-blkback-y	:= blkback.o xenbus.o
