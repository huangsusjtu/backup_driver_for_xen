#include <linux/fs.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/major.h>
#include "my_common.h"

static int user_cmd_ioctl( struct file* file, unsigned int cmd, unsigned long arg);
static int user_cmd_open(struct inode* inode, struct file* file);

static struct file_operations cmd_fops={
	.owner = THIS_MODULE,
	.open  = user_cmd_open,
	.unlocked_ioctl = user_cmd_ioctl,
};


static struct cdev dev;
static dev_t dev_number;

void init_user_cmd(void)
{
	if(alloc_chrdev_region(&dev_number,0,1,"xenbackup")<0 )
	{
		printk("Can not register device\n");
		return ;
	}
	printk("major device number %d\n",dev_number);
	cdev_init(&dev, &cmd_fops);
	dev.owner = THIS_MODULE;
	if( cdev_add(&dev, dev_number, 1) )
	{
		printk("Bad cdev\n");
		return ;
	}
	printk("Good user_cmd cdev\n");
}


void exit_user_cmd(void)
{
	cdev_del(&dev);
	unregister_chrdev_region(MAJOR(dev_number),1);
}


static int user_cmd_open(struct inode* inode, struct file* file)
{
	file->private_data = &flag;
	return 0;
}

static int user_cmd_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
	int *p =  (int*)(file->private_data); 
	switch (cmd)
	{
		case  RECODR:
			*p |= MASK_START; 
			printk("command 1 %x\n",flag);
			break;
			
		case  STOP:
			*p &= ~MASK_START;
			printk("command 2 %x\n",flag);
			break;

		case ROLLBACK:
			printk("command 3");
			return rollback(arg);	
		default:
			printk("Unkonw cmd %d from user\n",cmd);
			break;
	}
	return 0;
}

