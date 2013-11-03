#include "my_common.h"


static ssize_t write_blockfile_data(const void *buf,loff_t sector,size_t sector_per_bit,size_t n_sec);
static ssize_t read_blockfile_data(const void *buf,loff_t sector,size_t sector_per_bit,size_t n_sec);

struct backup_file_desc* file_desc = NULL;
struct record_file_desc* record_desc = NULL;


void init_file(const char* backupfilename,const char *recordfilename)  //初始化两个描述符
{
	struct inode *host;
	loff_t len = 4*sizeof(loff_t);

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
	if( ERR_PTR((long)record_desc->file_handle) == (void*)-2 ) //record文件不存在时.
	{
		//printk("no snapshot\n");
		file_desc->start = file_desc->end = 0;
		record_desc->file_handle = filp_open(recordfilename,O_CREAT|O_RDWR,0666);
		record_desc->start = len;
		record_desc->end = len;
		//printk("r_start:%lld r_end:%lld len:%lld\n",record_desc->start,record_desc->end,len);
		metadata_to_record();
		
	}else{
		//printk("read desc from snapshot\n");
		record_to_metadata();
		//printk("r_start:%lld r_end:%lld len:%lld\n",record_desc->start,record_desc->end,len);
	}
	file_desc->block_size = PAGE_SIZE;   //we define the size of block equal to page size.
	host = file_desc->file_handle->f_mapping->host;
	spin_lock(&host->i_lock);
	file_desc->n_blocks = host->i_blocks;
	spin_unlock(&host->i_lock);
	
}

 void metadata_to_record(void)
{
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
}

 void record_to_metadata(void)
{
	mm_segment_t fs;
	loff_t len;
	char buf[4*sizeof(loff_t)];
	loff_t pos = 0;
	len = 4*sizeof(loff_t);


	/*for(i=0;i<len;i++)
		printk("%x",buf[i]);
	printk("\n");*/
	//printk("r_start:%lld r_end:%lld len:%lld\n",record_desc->start,record_desc->end,len);
	fs = get_fs();
	set_fs(KERNEL_DS);
	vfs_read(record_desc->file_handle,(char*)buf,len,&pos);
	set_fs(fs);
	
	memcpy(&file_desc->start,&buf[0],sizeof(loff_t));
	memcpy(&file_desc->end,&buf[1*sizeof(loff_t)],sizeof(loff_t));
	memcpy(&record_desc->start,&buf[2*sizeof(loff_t)],sizeof(loff_t));
	memcpy(&record_desc->end,&buf[3*sizeof(loff_t)],sizeof(loff_t));
}



void exit_file(void)
{
	//printk("write desc to snapshot %d\n",pos);
	metadata_to_record();
	filp_close(file_desc->file_handle,NULL);
	filp_close(record_desc->file_handle,NULL);
	
	kfree(file_desc);
	kfree(record_desc);
	
}

static ssize_t write_blockfile_data( const void *buf,loff_t sector,size_t sector_per_bit,size_t n_sec )
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

static ssize_t read_blockfile_data( const void *buf,loff_t sector,size_t sector_per_bit,size_t n_sec )
{
	loff_t pos;
	size_t len;
	ssize_t ret;
	mm_segment_t fs;
	fs = get_fs();
	set_fs(KERNEL_DS);
	
	pos = sector<<sector_per_bit;
	len = n_sec<<sector_per_bit;
	
	ret = vfs_read(file_desc->file_handle,(char*)buf, len, &pos);
	
	set_fs(fs);
	return ret;
}



//////////////////////   page <-> blockfile
	

bool write_page_to_blockfile(struct page* page)
{
	void *buf;
	loff_t t;
	if(!page || !file_desc)
		return false;
	
	buf = kmap_atomic(page);
	
	spin_lock(&file_desc->lock);
	t=file_desc->start;
	++file_desc->start;
	if(file_desc->start == file_desc->n_blocks)
		file_desc->start = 0;
	if(file_desc->start==file_desc->end)
	{		
		++file_desc->end;
		if(file_desc->end == file_desc->n_blocks)
			file_desc->end = 0;
	}
	spin_unlock(&file_desc->lock);
	if(write_blockfile_data(buf,t,PAGE_SHIFT,1)<=0)
	{
		goto fail;
	}
	kunmap_atomic(buf);
	cond_resched();          
	return true;	

fail:
	kunmap_atomic(buf);
	return false;	
}

bool read_blockfile_to_page(struct page* page)
{
	void *buf;
	loff_t t;
	if(!page || !file_desc)
		return false;
	buf =  kmap_atomic(page);
	

	/*spin_lock(&file_desc->lock);		
	if(file_desc->start!=file_desc->end && read_blockfile_data(buf,file_desc->end,PAGE_SHIFT,1)>0)
	{
		t = ++file_desc->end;
		file_desc->end = do_div(t,file_desc->n_blocks);

		//printk("file_desc->start  in read_page_to_blockfile:%lld\n",file_desc->start);
		//printk("file_desc->end  in read_page_to_blockfile:%lld\n",file_desc->end);
		spin_unlock(&file_desc->lock);
		return true;
	}
	spin_unlock(&file_desc->lock);*/

	spin_lock(&file_desc->lock);	
	if(file_desc->start==file_desc->end)
	{	
		goto fail_unlock;
	}
	--file_desc->start;
	if(file_desc->start <0)
		file_desc->start = file_desc->n_blocks;
	t = file_desc->start;
	spin_unlock(&file_desc->lock);
	if(read_blockfile_data(buf,t,PAGE_SHIFT,1)<=0)
	{	
		goto fail;
	}
	kunmap_atomic(buf);
	cond_resched();          
	return true;	

fail_unlock:
	spin_unlock(&file_desc->lock);
fail:
	kunmap_atomic(buf);
	return false;
}
////////////////////////   record <-> recordfile

bool read_record(struct record* rec)
{
	mm_segment_t fs;
	loff_t lof;
	size_t len = sizeof(struct record);
	if(!record_desc || !rec)
		return false;

	/*spin_lock(&record_desc->lock);
	fs = get_fs();
	set_fs(KERNEL_DS);
	if(vfs_read(record_desc->file_handle,(char*)rec,sizeof(struct record),&record_desc->end)>0)
	{
		set_fs(fs);
		spin_unlock(&record_desc->lock);		
		return true;
	}
	set_fs(fs);
	spin_unlock(&record_desc->lock);*/
	
	spin_lock(&record_desc->lock);
	if(record_desc->start==record_desc->end)
	{
		spin_unlock(&record_desc->lock);
		return false;
	}
	record_desc->start -= len;
	lof = record_desc->start;
	spin_unlock(&file_desc->lock);
	fs = get_fs();
	set_fs(KERNEL_DS);
	while(vfs_read(record_desc->file_handle,(char*)rec,len,&lof)<=0);
	set_fs(fs);
	cond_resched();	
	return true;	
}

bool write_record(struct record* rec)
{
	mm_segment_t fs;
	loff_t lof;
	size_t len = sizeof(struct record);
	if(!record_desc)
		return false;
	if(!rec)
		return false;
	//printk("begin write record\n");
	//printk("record_desc in write_record:%d\n",record_desc);
	//printk("record_desc->file_handle in write_record:%d\n",record_desc->file_handle);
	//printk("record_desc->start in write_record:%lld\n",record_desc->start);
	/*spin_lock(&record_desc->lock);
	fs = get_fs();
	set_fs(KERNEL_DS);
	if(vfs_write(record_desc->file_handle,(char*)rec,sizeof(struct record),&record_desc->start)>0)
	{
		//printk("record_desc->start in write_record:%lld\n",record_desc->start);
		set_fs(fs);
		spin_unlock(&record_desc->lock);
		return true;
	}
	set_fs(fs);
	spin_unlock(&record_desc->lock);*/

	spin_lock(&record_desc->lock);
	lof = record_desc->start;
	record_desc->start += len;
	spin_unlock(&record_desc->lock);
	
	fs = get_fs();
	set_fs(KERNEL_DS);
	while(vfs_write(record_desc->file_handle,(char*)rec,len,&lof)<=0);	
	set_fs(fs);
	cond_resched();
	return true;
}


////////////////   timestamp
long get_time()
{
	struct timespec ts;
	getnstimeofday(&ts);
	return ts.tv_sec;
}











