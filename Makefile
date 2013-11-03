obj-m := xen-blkback.o
xen-blkback-y := blkback.o xenbus.o blockfile_rw.o page_pool.o rollback.o
#rw_workqueue.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

install:
	rm  -f /lib/modules/$(shell uname -r)/kernel/drivers/block/xen-blkback/xen-blkback.ko 
	mv ./xen-blkback.ko /lib/modules/$(shell uname -r)/kernel/drivers/block/xen-blkback/

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	rm -r /home/xen/domains/huang01/snapshot







#obj-$(CONFIG_XEN_BLKDEV_BACKEND) := xen-blkback.o

#xen-blkback-y	:= blkback.o xenbus.o

# 怎样添加宏MACRO_NAME到特定的源文件
# CFLAGS_object.o += -DMACRO_NAME

#怎样添加宏MY_DEBUG到全部文件
#EXTRA_CFLAGS += -DMY_DEBUG
