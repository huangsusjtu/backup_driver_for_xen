#include "my_common.h"


struct free_page_head page_head = {     .page =NULL,
					.length = 0 };

static void expand_pool(int size)
{
	int i=0;
	spin_lock(&page_head.page_lock);

	while(i<size)
	{
		page_head.page[page_head.length] = alloc_page(GFP_KERNEL);
		while(NULL==page_head.page[page_head.length])
			page_head.page[page_head.length] = alloc_page(GFP_KERNEL);
		i++;
		page_head.length++;
	}
	spin_unlock(&page_head.page_lock);
}

int page_pool_init(int size)
{
	if(page_head.length!=0)
		return INITED_HEAD;
	
	spin_lock_init(&page_head.page_lock);
	
	page_head.page = kzalloc(size*sizeof(struct page*),GFP_KERNEL);
	expand_pool(size);
	dprint("page pool init ok\n");
        return 0;
}

void page_pool_destory()
{
	int i;
	spin_lock(&page_head.page_lock);
	for(i=0;i<page_head.length;i++)
		__free_page(page_head.page[i]);
	page_head.length = 0;
	spin_unlock(&page_head.page_lock);
	dprint("page pool destroy ok\n");
}

struct page* get_free_page()
{
	struct page* p ;
	if(0==page_head.length)
		return NULL;
	spin_lock(&page_head.page_lock);
	p = page_head.page[--page_head.length];
	spin_unlock(&page_head.page_lock);
	return p;
}

void put_free_page(struct page* page)
{
	if(NULL==page)
		return ;

	spin_lock(&page_head.page_lock);
	page_head.page[page_head.length++] = page;
	spin_unlock(&page_head.page_lock);
}


