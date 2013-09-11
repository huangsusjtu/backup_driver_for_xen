#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/string.h>

#define ERR_HANDLE -1
#define MAGIC 0x12345678
#define INITED_HEAD -1
#define PAGE_SIZE 1<<12
#define FREE_PAGE_SIZE 128

struct backup_file_desc{                   //description for  backup file
	int magic;
	struct file* file_handle;
	loff_t start;
	loff_t end;
	spinlock_t lock;
	unsigned int block_size;
	uint64_t n_blocks; 
};

struct record_file_desc{                   //description for  record file
	struct file* file_handle;
	loff_t start;
	loff_t end;		
	spinlock_t lock;
};

struct record{                             // format for write into snapshot file
	long ts_nsec ;                     //timestamp
	uint64_t src ;
	uint64_t des ;
	unsigned int n_pages;                          
};


struct free_page_head{                        //page_pool
	struct page* page[FREE_PAGE_SIZE];
	spinlock_t page_lock;
	unsigned int length;
};


long get_time(void); // timestamp
void init_file(const char* backupfilename,const char *recordfilename) ; //初始化两个描述符
void exit_file(void);

bool read_record(struct record* rec);
bool write_record(struct record* rec);
bool read_blockfile_to_page(struct page* page);
bool write_page_to_blockfile(struct page* page);

int page_pool_init(void);
void page_pool_destory(void);
struct page* get_free_page(void);
void put_free_page(struct page* page);


extern struct free_page_head page_head;
extern struct record_file_desc* record_desc;
extern struct backup_file_desc* file_desc;

