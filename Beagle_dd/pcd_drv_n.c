#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s: " fmt,__func__

#define MAM_SIZE_MAX_PCDEV1 1024
#define MAM_SIZE_MAX_PCDEV2 512
#define MAM_SIZE_MAX_PCDEV3 1024
#define MAM_SIZE_MAX_PCDEV4 512

#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR 0x11

#define NO_OF_DEVICES 4

/* psedo devices memory */
char device_buffer_pcdev1[MAM_SIZE_MAX_PCDEV1];
char device_buffer_pcdev2[MAM_SIZE_MAX_PCDEV2];
char device_buffer_pcdev3[MAM_SIZE_MAX_PCDEV3];
char device_buffer_pcdev4[MAM_SIZE_MAX_PCDEV4];

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence);
ssize_t pcd_write(struct file * filp, const char __user * buff, size_t count, loff_t *f_pos);
ssize_t pcd_read(struct file * filp, char __user * buff, size_t count, loff_t *f_pos);
int pcd_release(struct inode *inode, struct file *filp);
int pcd_open(struct inode *inode, struct file *filp);
int check_permission(int dev_perm, int acc_mode);

/* device private data structure */
struct pcdev_private_data
{
	char *buffer;
	unsigned size;
	const char *serial_number;
	int perm;
	struct cdev cdev;
};

/* driver private data structure */
struct pcdrv_private_data
{
	int total_devices;
	/* This hold the device number */
	dev_t device_number;
	struct class *class_pcd;
	struct device *device_pcd;
	struct pcdev_private_data pcdev_data[NO_OF_DEVICES];
};

/* file operations of the driver*/
struct file_operations pcd_fops = {
	.owner = THIS_MODULE,
	.open  = pcd_open,
	.read = pcd_read,
	.write = pcd_write,
	.llseek= pcd_lseek,
	.release = pcd_release
};

struct pcdrv_private_data pcdrv_data = {
	.total_devices = NO_OF_DEVICES,
	.pcdev_data = {
		[0] = {
			.buffer = device_buffer_pcdev1,
			.size = MAM_SIZE_MAX_PCDEV1,
			.serial_number = "PCDEV1XYZ123",
			.perm = RDONLY
			},
			
		[1] = {
			.buffer = device_buffer_pcdev2,
			.size = MAM_SIZE_MAX_PCDEV2,
			.serial_number = "PCDEV2XYZ123",
			.perm = WRONLY
			},
		[2] = {
			.buffer = device_buffer_pcdev3,
			.size = MAM_SIZE_MAX_PCDEV3,
			.serial_number = "PCDEV2XYZ123",
			.perm = RDWR
			},
		[3] = {
			.buffer = device_buffer_pcdev4,
			.size = MAM_SIZE_MAX_PCDEV4,
			.serial_number = "PCDEV3XYZ123",
			.perm = 0x11 /*RDWR*/
			}
		}
};

/*Module's init entry point */
static int __init pcd_init(void)
{
	int ret;
	int i;
	pr_info("Hello from chr drv\n");
	
	/* 1. Dynamically allocate a device number*/
	ret = alloc_chrdev_region(&pcdrv_data.device_number, 0, NO_OF_DEVICES, "pcd_devices");
	if(ret < 0)
	{
		pr_info("alloc chrdev failed\n");
		goto out;
	}
	
		
	/* create device class under /sys/class/ */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	pcdrv_data.class_pcd = class_create("pcd_class");
	if(IS_ERR(pcdrv_data.class_pcd))
	{
		pr_err("class creation failed\n");
		ret = PTR_ERR(pcdrv_data.class_pcd);
		goto unreg_chrdev;
	}
#else
	pcdrv_data.class_pcd = class_create(THIS_MODULE, "pcd_class");
	if(IS_ERR(pcdrv_data.class_pcd))
	{
		pr_err("class creation failed\n");
		ret = PTR_ERR(pcdrv_data.class_pcd);
		goto unreg_chrdev;
	}
#endif
	
	for(i=0; i<NO_OF_DEVICES; i++)
	{
	pr_info("device number <major>:<minor> = %d:%d\n", MAJOR(pcdrv_data.device_number+i), MINOR(pcdrv_data.device_number+i));
	
	/* 2. Initialize the cdev structure with fops*/
	cdev_init(&pcdrv_data.pcdev_data[i].cdev, &pcd_fops);
	
	/*3. Register a device with vfs*/
	ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev, pcdrv_data.device_number+i, 1);
	if(ret < 0)
	{
		pr_err("cdev add failed\n");
		goto cdev_del;
	}

	
	/* populate the sysfs with device information */
	pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, pcdrv_data.device_number+i, NULL, "pcdev-%d",i+1);
	if(IS_ERR(pcdrv_data.device_pcd))
	{
		pr_err("device create failed");
		ret = PTR_ERR(pcdrv_data.device_pcd);
		goto class_del;
	}
	}
	
	pr_info("module init was successfull...\n");
	return 0;

cdev_del:	
class_del:
	for(; i>=0; i--)
	{
	device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number+i);
	cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}
	class_destroy(pcdrv_data.class_pcd);
unreg_chrdev:
	unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);
out:
	pr_info("Module insertion failed\n");
	
	return ret;
}

/*Module's cleanup entry point */
static void __exit pcd_cleanup(void)
{
	int i;
	for(i=0; i< NO_OF_DEVICES; i++)
	{
	device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number+i);
	cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}
	class_destroy(pcdrv_data.class_pcd);

	unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);
	pr_info("Good bye from chr drv\n");
}

int check_permission(int dev_perm, int acc_mode)
{
	if(dev_perm == RDWR)
		return 0;

	// ensures readonly access
	if((dev_perm == RDONLY) && ((acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE)))
		return 0;

	// ensures writeonly access
	if((dev_perm == WRONLY) && ((acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ)))
		return 0;

	return -EPERM;
}

int pcd_open(struct inode *inode, struct file *filp)
{
	pr_info("welcome to open\n");
	int ret;
	int minor_n;
	struct pcdev_private_data *pcdev_data;

	/* find out on which device file open was attempted by the user space */
	minor_n = MINOR(inode->i_rdev);
	pr_info("minor access = %d\n", minor_n);
	
	/* get device's pricvate data structure */
	pcdev_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev);
	
	filp->private_data = pcdev_data;
	
	/* check permissions */
	ret = check_permission(pcdev_data->perm, filp->f_mode);
	
	(!ret)?pr_info("open was successfull\n"):pr_info("open was unsuccessfull\n"); 
	
	return ret;
}

int pcd_release(struct inode *inode, struct file *filp)
{
	pr_info("release was successfull\n");
	return 0;
}

ssize_t pcd_read(struct file * filp, char __user * buff, size_t count, loff_t *f_pos)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
	int max_size = pcdev_data->size;
	
	pr_info("read requested for %zu bytes\n", count);
	pr_info("current file position= %lld\n", *f_pos);
	
	/*adjust the count*/
	if((*f_pos + count) > max_size)
	{
		count = max_size - *f_pos;
	}
	
	/*copy to user*/
	if(copy_to_user(buff, pcdev_data->buffer+(*f_pos), count))
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
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
	int max_size = pcdev_data->size;
	
	pr_info("write requested for %zu bytes\n", count);
	pr_info("current file position= %lld\n", *f_pos);
	
	/*adjust the count*/
	if((*f_pos + count) > max_size)
	{
		count = max_size - *f_pos;
	}
	
	if(!count)
	{
		return -ENOMEM;
	}
	
	/*copy from user*/
	if(copy_from_user(pcdev_data->buffer+(*f_pos), buff, count))
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
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
	int max_size = pcdev_data->size;
	loff_t temp;
	
	pr_info("lseek is requested\n");
	pr_info("current file position= %lld\n", filp->f_pos);

	switch(whence)
	{
		case SEEK_SET:
			if((offset > max_size) || (offset < 0))
				return -EINVAL;
			filp->f_pos = offset;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + offset;
			if((temp > max_size) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		case SEEK_END:
			temp = max_size + offset;
			if((temp > max_size) || (temp < 0))
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
