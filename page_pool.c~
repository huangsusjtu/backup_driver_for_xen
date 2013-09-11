#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/page.h>
#include <linux/slab.h>

struct free_page{
	struct list_head list;
	struct page* page;
};

struct free_page_head{
	struct list_head list;
	spinlock_t page_lock;
	unsigned int length;
};
struct free_page_head page_head = {     .page_lock = SPIN_LOCK_UNLOCK,
					.length = 0 };

#define FREE_PAGE_SIZE 100;

#define INITED_HEAD -1
#define PAGE_ALLOC_ERR -2
#define NO_FREE_PAGE -3


int page_pool_init()
{
	int i=0;
	struct free_page* entry;
	if(page_head.length>0)
		return INITED_HEAD;
	
	INIT_LIST_HEAD(&page_head.list);
	spin_lock(&page_head.page_lock);
	while(i<FREE_PAGE_SIZE)
	{
		entry = (struct free_page*)kmalloc(GFP_KERNEL,sizeof(struct free_page));
		if( (entry->page=alloc_page(GFP_KERNEL) )==NULL)
		{
			spin_unlock(&page_head.page_lock);
			goto fail_page_alloc;
		}
		list_add_tail(&entry->list,&page_head.list);	
		page_head.length++;
		i++;
	}
	spin_unlock(&page_head.page_lock);
        return 0;
fail_page_alloc:
	page_pool_destory();
	return PAGE_ALLOC_ERR;
	
}

void page_pool_destory()
{
	spin_lock(&page_head.page_lock);
	while(!list_empty(&page_head.list))
	{
		struct list_head* p = page_head.list.next;
		struct free_page* entry = list_entry(p,struct free_page,list);
		free_page(entry->page);
		free(entry);
		list_del(p);
	}
	spin_unlock(&page_head.page_lock);
}

struct page* get_free_page()
{
	struct page* res ;
	struct free_page* entry ;

	if(list_empty(page_head.list))
		return NULL;
        
	spin_lock(&page_head.page_lock);
	entry =  list_first_entry(page_head.list,struct free_page,list);
	res = entry->page;
	list_del(page_head.list->next);
	page_head.length--;
	spin_unlock(&page_head.page_lock);
	free(entry);
	
	return res;
}

void put_free_page(struct page* page)
{
	struct free_page* entry;
	
	if(page==NULL)
		return ;

	entry = (struct free_page*)kmalloc(GFP_KERNEL,sizeof(struct free_page));
	entry->page = page ;
	spin_lock(&page_head.page_lock);
	list_add(entry->list,page_head.list);
	page_head.length++;
	spin_unlock(&page_head.page_lock);
}


