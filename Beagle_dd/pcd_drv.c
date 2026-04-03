#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define DEV_MEM_SIZE 512
char device_buffer[DEV_MEM_SIZE];

/* This hold the device number */
dev_t device_number;

/* cdev variable*/
struct cdev pcd_cdev;

struct class *class_pcd;

struct device *device_pcd;

#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt,__func__

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence);
ssize_t pcd_write(struct file * filp, const char __user * buff, size_t count, loff_t *f_pos);
ssize_t pcd_read(struct file * filp, char __user * buff, size_t count, loff_t *f_pos);
int pcd_release(struct inode *inode, struct file *filp);
int pcd_open(struct inode *inode, struct file *filp);

/* file operations of the driver*/
struct file_operations pcd_fops = {
	.owner = THIS_MODULE,
	.open  = pcd_open,
	.read = pcd_read,
	.write = pcd_write,
	.llseek= pcd_lseek,
	.release = pcd_release
};

/*Module's init entry point */
static int __init pcd_init(void)
{
	int ret;
	pr_info("Hello from chr drv\n");
	
	/* 1. Dynamically allocate a device number*/
	ret = alloc_chrdev_region(&device_number, 0, 1, "pcd_devices");
	if(ret < 0)
	{
		pr_err("chardev failed\n");
		goto out;
	}
	
	pr_info("device number major={}, minor={}\n", MAJOR(device_number), MINOR(device_number));
	
	/* 2. Initialize the cdev structure with fops*/
	cdev_init(&pcd_cdev, &pcd_fops);
	
	/*3. Register a device with vfs*/
	ret = cdev_add(&pcd_cdev, device_number, 1);
	if(ret < 0)
	{
		pr_err("cdev add failed\n");
		goto unreg_chrdev;
	}
	
	/* create device class under /sys/class/ */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	class_pcd = class_create("pcd_class");
	if(IS_ERR(class_pcd))
	{
		pr_err("class creation failed\n");
		ret = PTR_ERR(class_pcd);
		goto cdev_del;
	}
#else
	class_pcd = class_create(THIS_MODULE, "pcd_class");
	if(IS_ERR(class_pcd))
	{
		pr_err("class creation failed\n");
		ret = PTR_ERR(class_pcd);
		goto cdev_del;
	}
#endif
	
	/* populate the sysfs with device information */
	device_pcd = device_create(class_pcd, NULL, device_number, NULL, "pcd");
	if(IS_ERR(device_pcd))
	{
		pr_err("device create failed\n");
		ret = PTR_ERR(device_pcd);
		goto class_del;
	}
	
	pr_info("module init was successfull...\n");
	
	return 0;

class_del:
	class_destroy(class_pcd);
cdev_del:
	cdev_del(&pcd_cdev);
unreg_chrdev:
	unregister_chrdev_region(device_number, 1);
out:
	pr_info("module insertion failed...\n");
	return ret;
}

/*Module's cleanup entry point */
static void __exit pcd_cleanup(void)
{
	device_destroy(class_pcd, device_number);
	class_destroy(class_pcd);
	cdev_del(&pcd_cdev);
	unregister_chrdev_region(device_number, 1);
	pr_info("Good bye from chr drv\n");
}

int pcd_open(struct inode *inode, struct file *filp)
{
	pr_info("open was successfull\n");
	return 0;
}

int pcd_release(struct inode *inode, struct file *filp)
{
	pr_info("release was successfull\n");
	return 0;
}

ssize_t pcd_read(struct file * filp, char __user * buff, size_t count, loff_t *f_pos)
{
	pr_info("read requested for %zu bytes\n", count);
	pr_info("current file position= %lld\n", *f_pos);
	
	/*adjust the count*/
	if((*f_pos + count) > DEV_MEM_SIZE)
	{
		count = DEV_MEM_SIZE - *f_pos;
	}
	
	/*copy to user*/
	if(copy_to_user(buff, &device_buffer[*f_pos], count))
	{
		return -EFAULT;
	}
	
	/*update the current file position*/
	*f_pos += count;
	
	pr_info("number of bytes successfully read= %zu bytes\n", count);
	pr_info("updated file position= %lld\n", *f_pos);
	
	/*return number of bytes which have been successfully read*/
	return count;
}

ssize_t pcd_write(struct file * filp, const char __user * buff, size_t count, loff_t *f_pos)
{
	pr_info("write requested for %zu bytes\n", count);
	pr_info("current file position= %lld\n", *f_pos);
	
	/*adjust the count*/
	if((*f_pos + count) > DEV_MEM_SIZE)
	{
		count = DEV_MEM_SIZE - *f_pos;
	}
	
	if(!count)
	{
		return -ENOMEM;
	}
	
	/*copy from user*/
	if(copy_from_user(&device_buffer[*f_pos], buff, count))
	{
		return -EFAULT;
	}
	
	/*update the current file position*/
	*f_pos += count;
	
	pr_info("number of bytes successfully written= %zu bytes\n", count);
	pr_info("updated file position= %lld\n", *f_pos);
	
	/*return number of bytes which have been successfully written*/
	return count;
}

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
	pr_info("lseek is requested\n");
	pr_info("current file position= %lld\n", filp->f_pos);
	loff_t temp;
	switch(whence)
	{
		case SEEK_SET:
			if((offset > DEV_MEM_SIZE) || (offset < 0))
				return -EINVAL;
			filp->f_pos = offset;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + offset;
			if((temp > DEV_MEM_SIZE) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		case SEEK_END:
			temp = DEV_MEM_SIZE + offset;
			if((temp > DEV_MEM_SIZE) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		default:
			return -EINVAL;
	}
	pr_info("updated file position= %lld\n", filp->f_pos);
	return filp->f_pos;
}

module_init(pcd_init);
module_exit(pcd_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jay");
MODULE_DESCRIPTION("A simple pcd kernel module");
MODULE_INFO(board,"Beaglebone black REV A5");
