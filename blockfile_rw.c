#include <linux/file.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include "common.h"

#define ERR_HANDLE -1


struct backup_file_desc{                   //description for  backup file
	const char name[32];
	struct file* file_handle;
	loff_t start;
	loff_t end;
	spinlock_t f_lock;
	unsigned int block_size;
	unsigned int n_blocks; 
};
struct backup_file_desc* file_desc=NULL;


struct record{                                // format for write into snapshot file
	long ts_nsec ;                        //timestamp
	size_t src ;
	size_t des ;
	unsigned int n_sector;                          
};



void init_file(const char* backupfilename)
{
	if(file_desc)
		return ;
	file_desc = kmalloc(GFP_KERNEL,sizeof(struct backup_file_desc));
	if(!file_desc)
		return ;
	file_desc->file_handle = filp_open(backupfilename,O_RDWR,0666);
	strcpy(file_desc->name,logfilename);
	file_desc->start = file_desc->end = 0;
	file_desc->lock = SPIN_LOCK_UNLOCK;	
	file_desc->block_size = 1<<12;
	file_desc->n_blocks = file_desc->f_mapping->host->i_blocks;	
}

void exit_file()
{
	filp_close(file_desc->file_handle,NULL);
}

static ssize_t write_blockfile_data( const void *buf,size_t sector,size_t sector_per_bit,size_t n_sec )
{
	loff_t pos;
	size_t len;
	ssize_t ret;
	mm_segment_t fs;
	fs = get_fs();
	set_fs(KERNEL_DS);
	
	pos = sector<<sector_per_bit;
	len = n_sec<<sector_per_bit;
	if(!file_handle)
		return ERR_HANDLE;
	ret = vfs_write(file_handle, buf, len, &pos);
	
	set_fs(fs);
	return ret;
}

static ssize_t read_blockfile_data( const void *buf,size_t sector,size_t sector_per_bit,size_t n_sec )
{
	loff_t pos;
	size_t len;
	ssize_t ret;
	mm_segment_t fs;
	fs = get_fs();
	set_fs(KERNEL_DS);
	
	pos = sector<<sector_per_bit;
	len = n_sec<<sector_per_bit;
	if(!file_handle)
		return ERR_HANDLE;
	ret = vfs_read(file_handle, buf, len, &pos);
	
	set_fs(fs);
	return ret;
}



//////////////////////
	

bool write_page_to_blockfile(struct page* page)
{
	void *buf;
	if(!page)
		return false;
	if(page->virtual!=NULL)
	{
		buf = page->virtual;						
	}else{
		buf =pfn_to_kaddr(page_to_pfn(page));
	}
	if(!file_desc)
		return false;
	
	if(write_blockfile_data(buf,file_desc->start,12,1)>0)
	{
		file_desc->start++;
		file_desc->start |= (file_desc->n_blocks-1);
		if(file_desc->start==file_desc->end)
		{		
			file_desc->end++;
		file_desc->end |= (file_desc->n_blocks-1);
		}
		return true;
	}
	return false;
}

bool read_blockfile_to_page(struct page* page)
{
	void *buf;
	if(!page)
		return false;
	if(page->virtual!=NULL)
	{
		buf = page->virtual;						
	}else{
		buf =pfn_to_kaddr(page_to_pfn(page));
	}
	if(!file_desc)
		return false;
	
	if(write_blockfile_data(buf,file_desc->start,12,1)>0)
	{
		file_desc->start++;
		file_desc->start |= (file_desc->n_blocks-1);
		if(file_desc->start==file_desc->end)
		{		
			file_desc->end++;
		file_desc->end |= (file_desc->n_blocks-1);
		}
		return true;
	}
	return false;
}













