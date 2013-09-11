#include <linux/file.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include "my_common.h"


struct file* gloable_file_handle = NULL;
loff_t pos=0;

void init_log_file(const char* logfilename)
{
	if(!gloable_file_handle)
		gloable_file_handle = filp_open(logfilename,O_RDWR|O_CREAT,0666);
}

void exit_log_file()
{
	filp_close(gloable_file_handle,NULL);
}

ssize_t write_log(const char *fmt, ...)
{
	char buf[1024];
	int len;
	
	mm_segment_t fs;
	va_list args;
	
	//memset(buf,0,1024);
	va_start(args,fmt);
	vsprintf(buf,fmt,args);
	if(NULL == gloable_file_handle)
		return -1;
	fs = get_fs();
	set_fs(KERNEL_DS);
 	len = strlen(buf);
	vfs_write(gloable_file_handle,buf,len,&pos);
	set_fs(fs);
	return pos;
}

static int file_log_init()
{
	struct page* p = NULL;
	char *ptr = NULL;
	char a =0x00;
	int i=0;
	init_log_file("/home/huangsu/tet/debuglog");
	/*if(page_pool_init()==0){
		write_log("page pool init: ok");
		write_log("page pool length %d\n: ok",page_head.length);
		struct page* p  = get_free_page();
		if(p){
			write_log("get page: ok\n");
			write_log("get page address: %d\n", page_address(p));
					
		}else{
			write_log("get page: error\n");
		}
		

		page_pool_destory();
	}
	exit_log_file();
*/
	write_log("huangsu: %d\n",12);

	
	init_file("/home/xen/domains/huang01/back.img","/home/xen/domains/huang01/snapshot");
	//write_log("file_dec    start:%lld  end:%lld \n",file_desc->start,file_desc->end);
	//write_log("record_desc start:%lld  end:%lld \n",record_desc->start,record_desc->end);
	
	page_pool_init();
	p = get_free_page();
	if(p)
	{
		printk("get page ok\n");
		ptr = page_address(page);
		if(ptr)
		{
			printk("get page address %d\n",ptr);
			for(i=0;i<256;i++)
				ptr[i] = a++;
			write_page_to_blockfile(p);
		}		
	}
	
	return 0;
}
static int file_log_exit()
{
	exit_file();
	exit_log_file();
	page_pool_destory();
	return 0;
}

module_init(file_log_init);
module_exit(file_log_exit);
