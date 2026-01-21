#include <linux/kernel.h> // printk
#include <linux/module.h> // module_license, module_author, module_description
#include <linux/init.h> // module_init, module_exit
#include <linux/fs.h> // open, read, write, ioctl, release, struct file_operations, register_chrdev_region, unregister_chrdev_region, register_chrdev, alloc_chrdev_region
#include <linux/kdev_t.h> // dev_t, major, minor, mkdev
#include <linux/cdev.h> // cdev_init, cdev_del, cdev_add, cdev_alloc, struct cdev
#include <linux/device.h> // device_create, class_create, struct device, struct class, device_destroy
#include <linux/slab.h> // kmalloc, kfree
#include <linux/uaccess.h> // copy_to_user, copy_from_user
#include <linux/ioctl.h> // Defines dictionary for own custom commands

//Define the ioctl code, 1.magic number, 2.sequence number, 3.size, Directions: _IOW: write, _IO: None, _IOR: Read, _IOWR: Both  
#define WR_DATA _IOW('a', 'a', int32_t*)
#define RD_DATA _IOR('a', 'b', int32_t*)

#define mem_size 1024
#define MY_MAJOR 303 // use while static allocation, check this number if it's not allocated to another device: use cat /proc/devices 
#define MY_MINOR 0 // use while static allocation
#define DEVICE_COUNT 1 // use while static allocation

static int __init chr_driver_init(void);
static void __exit chr_driver_exit(void);
static long chr_ioctl(struct file *file, unsigned int cmd, unsigned long arg);


int32_t val = 0;
static dev_t my_device_id; // use while static allocation
//dev_t dev = 0; // use while dynamic allocation
static struct class *dev_class;
static struct cdev my_cdev;
uint8_t *kernel_buffer;

static struct file_operations fops =
{
	.owner = THIS_MODULE,
	.unlocked_ioctl = chr_ioctl
};

static long chr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd)
	{
		case WR_DATA:
				copy_from_user(&val, (int32_t*)arg, sizeof(val));
				printk(KERN_INFO "val = %d\n", val);
				break;
		case RD_DATA:
				copy_to_user((int32_t*)arg, &val, sizeof(val));
				break;
	}
return 0;
}

static int __init chr_driver_init(void)
{
	int ret;

	////////////////////////////////////////////////////////////////////////////////////////////
	/* Allocating Major Number Dynamically*/
	//ret = alloc_chrdev_region(&dev, 0, 1, "my_dev");
	//if(ret < 0)
	//{
	//	printk(KERN_INFO "Cannot alocate the major number....\n ");
	//	return -1;
	//}
	/////////////////////////////////////////////////////////////////////////////////////////////

	/* Allocating Major Number Statically */
	my_device_id= MKDEV(MY_MAJOR, MY_MINOR);
	ret = register_chrdev_region(my_device_id, DEVICE_COUNT, "my_dev");
	if(ret < 0)
	{
		printk(KERN_INFO "Cannot register the major number....\n");
		return -1;
	}

	///////////////////////////////////////////////////////////////////////////////////////////

	// printk(KERN_INFO "Major = %d Minor=%d...\n", MAJOR(dev), MINOR(dev)); //enable while using dynamic allocation
	printk(KERN_INFO "Major = %d Minor = %d ...\n", MY_MAJOR, MY_MINOR); //enable while using static allocation

	/* Create cdev structure */
	cdev_init(&my_cdev, &fops);

	/*Add character device to the system*/
	//if((cdev_add(&my_cdev, dev, 1)) < 0) // enable while using dynamic allocation
	if((cdev_add(&my_cdev, my_device_id, DEVICE_COUNT)) < 0) //enable while using static allocation
	{
		printk(KERN_INFO "Cannot add the device to the system...\n");
		goto r_class;
	}

	/*Create struct class*/
	dev_class = class_create("test_class");
	if(dev_class == 0)
	{
		printk(KERN_INFO "Cannot create the struct class...\n");
		goto r_class;
	}

	/*Create device*/
	if((device_create(dev_class, NULL, my_device_id, NULL, "my_device")) == NULL) // enable while using static allocation
	// if((device_create(dev_class, NULL, dev, NULL, "my_device")) == NULL) // enable while using dyamic allocation
	{
		printk(KERN_INFO "Cannot create the device\n");
		goto r_device;
	}
	printk(KERN_INFO "Device driver insert.... done properly...\n");
	return 0;

r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(my_device_id, DEVICE_COUNT); // enable while using static allocation
	// unregister_chrdev_region(dev,1); // enable while using dynamic allocation
	return -1;
}

static void __exit chr_driver_exit(void)
{
	device_destroy(dev_class, my_device_id); // enable while using static allocation
	//device_destroy(dev_class,dev); // enable while using dynamic allocation
	class_destroy(dev_class);
	cdev_del(&my_cdev);
	unregister_chrdev_region(my_device_id, 1); // enable while using static allocation
	//unregister_chrdev_region(dev,1); // enable while using dynamic allocation
	printk(KERN_INFO "Device driver is removed successfully....\n");
}

module_init(chr_driver_init);
module_exit(chr_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jay");
MODULE_DESCRIPTION("My first ioctl Device Driver");
