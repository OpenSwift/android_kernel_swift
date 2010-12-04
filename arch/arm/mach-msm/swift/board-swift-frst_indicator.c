#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/cdev.h>

#include <asm/uaccess.h>
#include <asm/io.h>


#include "../smd_private.h"
#include "../smd_rpcrouter.h"

#include "board-swift-frst_indicator.h"




static struct mutex api_lock;

static int frstindicator_open(struct inode *inode, struct file *filp)
{
	printk("[eternalblue] frstindicator_open...\n");
	return 0;
}

static int frstindicator_release(struct inode *inode, struct file *filp)
{
	printk("[eternalblue] frstindicator_released\n");
	return 0;
}

static int frstindicator_ioctl(struct inode *inode, struct file *filp,
							unsigned int cmd, unsigned long arg)
{
	int 	err, size;
	int loop;
	static uint32_t frstindicator_addr;
	FactoryReset_Indicator *fr;
	int rc = -EIO;
//	printk("[eternalblue] frstindicator_ioctl1 CMD [%d] TYPE[%d]\n", cmd, _IOC_TYPE(cmd));

	if (_IOC_TYPE(cmd) != FRST_INDICATOR_MAGIC ) return -EINVAL;
//	printk("[eternalblue] frstindicator_ioctl2 CMD [%d] NR[%d]\n", cmd, _IOC_NR(cmd));
	if (_IOC_NR(cmd) >= FRST_INDICATOR_MAXNR) return -EINVAL;

//	printk("[eternalblue] frstindicator_ioctl3 [%d]\n", cmd);
	size = _IOC_SIZE(cmd);
//	printk("[eternalblue] frstindicator_ioctl4 [%d]\n", size);

	mutex_lock(&api_lock);
	frstindicator_addr = 
		(uint32_t)smem_alloc(SMEM_FACTORY_RESET_INDICATOR,
							sizeof(FactoryReset_Indicator));
//	printk("[eternalblue] frstindicator_ioctl5 [0x%8x]\n", frstindicator_addr);
	
	fr = frstindicator_addr;
	if (!frstindicator_addr) {
		printk("open failed: smem_alloc error\n");
		goto done;
	}		
	switch(cmd)
	{
		case FRST_INDICATOR_READ:
//			printk("[eternalblue] FRST_INDICATOR_READ arm9[%d] arm11[%d] frstatus[%d] frongoing[%d]\n",	fr->arm9_written_flag, fr->arm11_written_flag, fr->frststatus, fr->frstongoing);
			copy_to_user( ( void *) arg, (const void *) frstindicator_addr, (unsigned long ) sizeof(FactoryReset_Indicator));
			break;
		case FRST_INDICATOR_WRITE:
			copy_from_user( (void *) frstindicator_addr, (const void *) arg, size);
//			printk("[eternalblue] FRST_INDICATOR_WRITE arm9[%d] arm11[%d] frstatus[%d] frongoing[%d]\n",				fr->arm9_written_flag, fr->arm11_written_flag, fr->frststatus, fr->frstongoing);
			break;
	}
	rc = 0;	
done:
	mutex_unlock(&api_lock);
	return rc;	
}


struct file_operations frstindicator_fops =
{
	.owner	= THIS_MODULE,
	.ioctl		= frstindicator_ioctl,
	.open	= frstindicator_open,
	.release 	= frstindicator_release,
};
static dev_t frstindicator_device_number;
static struct cdev *frstindicator_device;

static int __init frstindicator_init(void)
{
	printk("frst_indicator init start\n");

	
	int result;
/*
	result = register_chrdev(FRSTINDICATOR_DEV_MAJOR,
						FRSTINDICATOR_DEV_NAME,
						&frstindicator_fops);
*/
	frstindicator_device_number = MKDEV(FRSTINDICATOR_DEV_MAJOR, FRSTINDICATOR_DEV_MINOR);
	register_chrdev_region(frstindicator_device_number, 1, FRSTINDICATOR_DEV_NAME);

	frstindicator_device = cdev_alloc();
	cdev_init(frstindicator_device, &frstindicator_fops);
	cdev_add(frstindicator_device, frstindicator_device_number, 1);
	
	printk("frst_indicator init MAJOR [%d] MINOR[%d]\n", MAJOR(frstindicator_device_number), MINOR(frstindicator_device_number));
	
//	if ( result <0 ) return result;

	mutex_init(&api_lock);

	return 0;
}

static void __exit frstindicator_exit(void)
{
//	unregister_chrdev(FRSTINDICATOR_DEV_MAJOR,FRSTINDICATOR_DEV_NAME);
	cdev_del(frstindicator_device);
	unregister_chrdev_region(frstindicator_device,1 );
}
module_init(frstindicator_init);
module_exit(frstindicator_exit);

MODULE_DESCRIPTION("Factory reset indicator for eternalblue");
MODULE_LICENSE("GPL");

