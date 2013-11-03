#include "my_common.h"
#include "common.h"
struct block_device* bd = NULL;

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


bool rollback(long deta_time)
{
	long current_time,roll_time;	
	struct record rec;
	struct bio* bio;
	int npages ;
	current_time = get_time();
	
	if(deta_time<0 || deta_time>current_time || !bd)	
		return false;
	roll_time = current_time - deta_time;
	
	while(true)
	{	
		if(!read_record(&rec))
		{
			return false;
		}
		if( rec.ts_nsec < roll_time)
		{
			write_record(&rec);
			return true;
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
}







