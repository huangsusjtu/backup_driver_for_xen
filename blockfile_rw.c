#include "my_common.h"


struct backup_file_desc* file_desc = NULL;
struct record_file_desc* record_desc = NULL;


void init_file(const char* backupfilename,const char *recordfilename)  //初始化两个描述符
{
	int i=0;
	char buf[4*sizeof(loff_t)];
	loff_t len;
	loff_t pos=0;
	mm_segment_t fs;
	len = 4*sizeof(loff_t);	
	
	if(file_desc || record_desc)
		return ;
	
	file_desc = kmalloc(GFP_KERNEL,sizeof(struct backup_file_desc));
	if(!file_desc)
		return ;
	record_desc = kmalloc(GFP_KERNEL,sizeof(struct record_file_desc));
	if(!record_desc)
		return;
	

	file_desc->file_handle = filp_open(backupfilename,O_RDWR,0666);
	spin_lock_init(&file_desc->lock);	
	record_desc->file_handle = filp_open(recordfilename,O_RDWR,0666);
	spin_lock_init(&record_desc->lock);
	
	//printk("file handle of record : %d\n",record_desc->file_handle);
	//printk("file handle of FILE_DESC : %d\n",file_desc->file_handle);
	
	//printk("len:%d\n",len);
	if( ERR_PTR(record_desc->file_handle) == -2) //record文件不存在时.
	{
		printk("no snapshot\n");
		file_desc->start = file_desc->end = 0;
		record_desc->file_handle = filp_open(recordfilename,O_CREAT|O_RDWR,0666);
		record_desc->start = len;
		record_desc->end = len;
		//printk("r_start:%lld r_end:%lld len:%lld\n",record_desc->start,record_desc->end,len);
		
	}else{
		printk("read desc from snapshot\n");
		fs = get_fs();
		set_fs(KERNEL_DS);
		vfs_read(record_desc->file_handle,(char*)buf,len,&pos);
		set_fs(fs);
		/*for(i=0;i<len;i++)
			printk("%x",buf[i]);
		printk("\n");*/	

		memcpy(&file_desc->start,&buf[0],sizeof(loff_t));
		memcpy(&file_desc->end,&buf[1*sizeof(loff_t)],sizeof(loff_t));
		memcpy(&record_desc->start,&buf[2*sizeof(loff_t)],sizeof(loff_t));
		memcpy(&record_desc->end,&buf[3*sizeof(loff_t)],sizeof(loff_t));
		//printk("r_start:%lld r_end:%lld len:%lld\n",record_desc->start,record_desc->end,len);
	}
	
	file_desc->block_size = 1<<12;
	file_desc->n_blocks = file_desc->file_handle->f_mapping->host->i_blocks;


}

void exit_file()
{
	int i=0;
	mm_segment_t fs;
	loff_t len;
	char buf[4*sizeof(loff_t)];
	loff_t pos = 0;
	len = 4*sizeof(loff_t);
	memcpy(&buf[0],&file_desc->start,sizeof(loff_t));
	memcpy(&buf[1*sizeof(loff_t)],&file_desc->end,sizeof(loff_t));
	memcpy(&buf[2*sizeof(loff_t)],&record_desc->start,sizeof(loff_t));
	memcpy(&buf[3*sizeof(loff_t)],&record_desc->end,sizeof(loff_t));

	/*for(i=0;i<len;i++)
		printk("%x",buf[i]);
	printk("\n");*/
	//printk("r_start:%lld r_end:%lld len:%lld\n",record_desc->start,record_desc->end,len);
	fs = get_fs();
	set_fs(KERNEL_DS);
	vfs_write(record_desc->file_handle,(char*)buf,len,&pos);
	set_fs(fs);

	//printk("write desc to snapshot %d\n",pos);

	filp_close(file_desc->file_handle,NULL);
	filp_close(record_desc->file_handle,NULL);
	
	kfree(file_desc);
	kfree(record_desc);
	
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
	ret = vfs_write(file_desc->file_handle, buf, len, &pos);
	
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
	ret = vfs_read(file_desc->file_handle,(const char*)buf, len, &pos);
	
	set_fs(fs);
	return ret;
}



//////////////////////
	

bool write_page_to_blockfile(struct page* page)
{
	void *buf;
	if(!page)
		return false;
	
	buf =pfn_to_kaddr(page_to_pfn(page));

	if(!file_desc)
		return false;
	
	if(write_blockfile_data(buf,file_desc->start,12,1)>0)
	{
		spin_lock(&file_desc->lock);
		file_desc->start++;
		file_desc->start |= (file_desc->n_blocks-1);
		if(file_desc->start==file_desc->end)
		{		
			file_desc->end++;
			file_desc->end |= (file_desc->n_blocks-1);
		}
		spin_unlock(&file_desc->lock);
		return true;
	}
	return false;
}

bool read_blockfile_to_page(struct page* page)
{
	void *buf;
	if(!page)
		return false;
	
	buf =pfn_to_kaddr(page_to_pfn(page));
	
	if(!file_desc)
		return false;
	
	if(read_blockfile_data(buf,file_desc->end,12,1)>0)
	{
		spin_lock(&file_desc->lock);
		file_desc->end++;
		file_desc->end |= (file_desc->n_blocks-1);
		if(file_desc->start==file_desc->end)
		{		
			file_desc->start++;
			file_desc->start |= (file_desc->n_blocks-1);
		}
		spin_unlock(&file_desc->lock);
		return true;
	}
	return false;
}
////////////////////////

bool read_record(struct record* rec)
{
	if(!record_desc)
		return false;
	if(!rec)
		return false;
	if(vfs_read(record_desc->file_handle,rec,sizeof(struct record),&record_desc->end)>0)
	{
		return true;
	}
	return false;
	
}

bool write_record(struct record* rec)
{
	if(!record_desc)
		return false;
	if(!rec)
		return false;
	if(vfs_write(record_desc->file_handle,rec,sizeof(struct record),&record_desc->start)>0)
	{
		return true;
	}
	return false;
}




////////////////
long get_time()
{
	struct timespec ts;
	getnstimeofday(&ts);
	return ts.tv_sec;
}










