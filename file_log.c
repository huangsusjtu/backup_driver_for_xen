#include <linux/file.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include "common.h"


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
/*
static int file_log_init()
{
	open_log_file("/home/huangsu/debuglog");
	write_log("huangsu: %d\n",12);
	close_log_file();
	return 0;
}
static int file_log_exit()
{}
*/
//module_init(file_log_init);
//module_exit(file_log_exit);
