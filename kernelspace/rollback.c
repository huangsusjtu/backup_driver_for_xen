#include "my_common.h"
#include "common.h"
struct block_device* bd = NULL;

int flag = 1;


/*MY code */
static void read_end_block_io_op(struct bio *bio, int error)
{
	int i=0;	
	struct bio_vec *bvec;
	struct record rec;
	atomic_t *cnt = bio->bi_private;
//	printk("Bio end1, and npages:%d\n",bio->bi_vcnt);
	
	bio_for_each_segment(bvec, bio, i)
	{
		write_page_to_blockfile(bvec->bv_page);
//		printk("write_page_to_blockfile i=%d ",i);
	}
	
	rec.ts_nsec = get_time();
	rec.src = bio->bi_sector;
	rec.des = file_desc->start;
	rec.n_pages = bio->bi_vcnt;
	write_record(&rec);
	if (atomic_dec_and_test(cnt)) {
		metadata_to_record();		
	}

	bio_put(bio);
//	printk("Bio end3");
}
	
//mycode




static void end_block_io_op(struct bio *bio, int error)
{
	struct bio_vec *bvec;
	int i=0;
	bio_for_each_segment(bvec, bio, i)
	{
		put_free_page(bvec->bv_page);
		printk("free pages of rollback\n");
	}

	bio_put(bio);
}


void hook_write(struct bio** biolist, int nbio)
{
		int i;
		int npages ;
		struct bio* bio;
		atomic_t pendcnt;
		printk("Write...........%d and diskname:%s\n",nbio,biolist[0]->bi_bdev->bd_disk->disk_name);
		atomic_set(&pendcnt, nbio);
		for (i = 0; i < nbio; i++)
		{
			if(!memcmp(biolist[i]->bi_bdev->bd_disk->disk_name,"loop0",5))
			{			
				//printk("i=:%d  biosize:%d",i,biolist[i]->bi_size);
				npages = biolist[i]->bi_size>>PAGE_SHIFT;
			
				bio = NULL;
				while(NULL==bio)			
					bio = bio_alloc(GFP_KERNEL,npages);

				bio->bi_bdev   = biolist[i]->bi_bdev;
				bio->bi_end_io  =  read_end_block_io_op;//end_block_io_op;
				bio->bi_sector  = biolist[i]->bi_sector;
				bio->bi_private = &pendcnt;
				while(npages>0)
				{
					struct page* p = get_free_page();
					//printk("bio npages %d\n",npages);
					//printk("get page address %d\n",page_address(p));
				
					bio_add_page(bio,p,PAGE_SIZE,0);
					npages--;
				}
				submit_bio(READ,bio);
			
			}
			//printk("out of for.....................................\n");
			//	while(!atomic_read(&pendcnt));
			//printk("after xen_blk_drain_io\n");
		}
}

int  rollback(long deta_time)
{
	long current_time,roll_time;	
	struct record rec;
	struct bio* bio;
	int npages ;
	current_time = get_time();
	
	if(deta_time<0 || deta_time>current_time || !bd)	
		return -1;
	roll_time = current_time - deta_time;
	
	while(true)
	{	
		if(!read_record(&rec))
		{
			return -2;
		}
		if( rec.ts_nsec < roll_time)
		{
			write_record(&rec);
			return 0;
		}
		
		npages = rec.n_pages;
		bio = NULL;
		while(NULL==bio)			
			bio = bio_alloc(GFP_KERNEL,npages);

		bio->bi_bdev   = bd;
		bio->bi_end_io  =  end_block_io_op;//end_block_io_op;
		bio->bi_sector  = rec.src;
		//bio->bi_private = ;
		while(npages>0)
		{
			struct page* p = get_free_page();
			//printk("bio npages %d\n",npages);
			//printk("get page address %d\n",page_address(p));
			if(!read_blockfile_to_page(p))
			{
				bio_put(bio);
				put_free_page(p);
			}
			bio_add_page(bio,p,PAGE_SIZE,0);
			npages--;
		}
		submit_bio(WRITE_ODIRECT,bio);

	}
	return 0;
}







