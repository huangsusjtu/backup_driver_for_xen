#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/gfp.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/file.h>
#include <linux/string.h>
#include <linux/sched.h>


#define ERR_HANDLE -1
#define INITED_HEAD -1
//#define PAGE_SHIFT 12
//#define PAGE_SIZE 1<<12
#define FREE_PAGE_SIZE 128

#define DEBUG_CODE

#ifdef DEBUG_CODE
#define dprint(a) printk(a)
#else 
#define dprint(a)
#endif


struct backup_file_desc{                   //description for backup file
	struct file* file_handle;
	loff_t start;          //u64
	loff_t end;
	spinlock_t lock;
	unsigned int block_size;
	blkcnt_t n_blocks;   //u64
};

struct record_file_desc{                   //description for record file
	struct file* file_handle;
	loff_t start;
	loff_t end;		
	spinlock_t lock;
};

struct record{                             //format for write into snapshot file
	long ts_nsec ;                     //timestamp
	loff_t src ;
	loff_t des ;
	unsigned int n_pages;                          
};


struct free_page_head{                        //page_pool
	struct page** page;
	spinlock_t page_lock;
	unsigned int length;
};


long get_time(void); // timestamp
void init_file(const char* backupfilename,const char *recordfilename) ; //初始化两个描述符
void exit_file(void);

void metadata_to_record(void);
void record_to_metadata(void);
bool read_record(struct record* rec);
bool write_record(struct record* rec);
bool read_blockfile_to_page(struct page* page);
bool write_page_to_blockfile(struct page* page);

int page_pool_init(int size);
void page_pool_destory(void);
struct page* get_free_page(void);
void put_free_page(struct page* page);


extern struct free_page_head page_head;
extern struct record_file_desc* record_desc;
extern struct backup_file_desc* file_desc;

