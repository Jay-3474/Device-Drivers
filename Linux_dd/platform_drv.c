#include <linux/kernel.h> // printk
#include <linux/module.h> // module_license, module_author, module_description
#include <linux/init.h> // module_init, module_exit
#include <linux/fs.h> // open, read, write, ioctl, release, struct file_operations, register_chrdev_region, unregister_chrdev_region, register_chrdev, alloc_chrdev_region
#include <linux/kdev_t.h> // dev_t, major, minor, mkdev
#include <linux/cdev.h> // cdev_init, cdev_del, cdev_add, cdev_alloc, struct cdev
#include <linux/device.h> // device_create, class_create, struct device, struct class, device_destroy
#include <linux/slab.h> // kmalloc, kfree
#include <linux/uaccess.h> // copy_to_user, copy_from_user
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/mod_devicetable.h>

#define mem_size 1024
#define MY_MAJOR 303 // use while static allocation, check this number if it's not allocated to another device: use cat /proc/devices 
#define MY_MINOR 0 // use while static allocation
#define DEVICE_COUNT 1 // use while static allocation

static int __init sample_init_gpio(void);
static void __exit sample_cleanup_gpio(void);
static int my_open(struct inode *inode, struct file *file);
static int my_release(struct inode *inode, struct file *file);
static ssize_t my_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t my_write(struct file *filp, const char __user *buf, size_t len, loff_t *off);

#define DRIVER_NAME "Sample_pldrv"
static unsigned int gpio_number = 0;
//static dev_t my_device_id; // use while static allocation
static dev_t first; // use while dynamic allocation
static struct class *dev_class;
static struct cdev cdev;

static struct file_operations fops =
{
	.owner = THIS_MODULE,
	.read = my_read,
	.write = my_write,
	.open = my_open,
	.release = my_release
};

static int my_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "Device File opened...\n");
	return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "Device FILE closed...\n");
	return 0;
}

static ssize_t my_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
	unsigned char temp = gpio_get_value(gpio_number);
	if(copy_to_user(buf, &temp, 1))
	{
		return -EFAULT;
	}
	printk(KERN_INFO "Data read: DONE...\n");
	return len;
}

static ssize_t my_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
	char temp;
	if(copy_from_user(&temp, buf, len))
	{
		return -EFAULT;
	}
	printk(KERN_INFO "In write call...\n");
	switch(temp)
	{
		case '0':
			gpio_set_value(gpio_number, 0);
			break;
		case '1':
			gpio_set_value(gpio_number, 1);
			break;
		default:
			printk(KERN_INFO "wrong option is entered...\n");
			break;
	}
	return len;
}

static int sample_drv_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;
	printk(KERN_ALERT "In probe function call....\n");
	
	of_property_read_u32(np, "led_number", &gpio_number);

	////////////////////////////////////////////////////////////////////////////////////////////
	/* Allocating Major Number Dynamically*/
	ret = alloc_chrdev_region(&first, 0, 1, "gpio_dev");
	if(ret < 0)
	{
		printk(KERN_INFO "Cannot alocate the major number....\n ");
		return -1;
	}
	/////////////////////////////////////////////////////////////////////////////////////////////

	/* Allocating Major Number Statically */
	//my_device_id= MKDEV(MY_MAJOR, MY_MINOR);
	//ret = register_chrdev_region(my_device_id, DEVICE_COUNT, "my_dev");
	//if(ret < 0)
	//{
	//	printk(KERN_INFO "Cannot register the major number....\n");
	//	return -1;
	//}

	///////////////////////////////////////////////////////////////////////////////////////////

	printk(KERN_INFO "Major = %d Minor=%d...\n", MAJOR(first), MINOR(first)); //enable while using dynamic allocation
	//printk(KERN_INFO "Major = %d Minor = %d ...\n", MY_MAJOR, MY_MINOR); //enable while using static allocation

	/* Create cdev structure */
	cdev_init(&cdev, &fops);

	/*Add character device to the system*/
	if((cdev_add(&cdev, first, 1)) < 0) // enable while using dynamic allocation
	//if((cdev_add(&my_cdev, my_device_id, DEVICE_COUNT)) < 0) //enable while using static allocation
	{
		printk(KERN_INFO "Cannot add the device to the system...\n");
		goto r_class;
	}

	/*Create struct class*/
	dev_class = class_create("gpiodrv");
	if(dev_class == 0)
	{
		printk(KERN_INFO "Cannot create the struct class...\n");
		goto r_class;
	}

	/*Create device*/
	//if((device_create(dev_class, NULL, my_device_id, NULL, "my_device")) == NULL) // enable while using static allocation
	if((device_create(dev_class, NULL, first, NULL, "gpio_device")) == NULL) // enable while using dyamic allocation
	{
		printk(KERN_INFO "Cannot create the device\n");
		goto r_device;
	}
	printk(KERN_INFO "Device driver insert.... done properly...\n");
	return 0;

r_device:
	class_destroy(dev_class);
r_class:
	//unregister_chrdev_region(my_device_id, DEVICE_COUNT); // enable while using static allocation
	 unregister_chrdev_region(first,1); // enable while using dynamic allocation
	return -1;
}

static void sample_drv_remove(struct platform_device *pdev)
{
	cdev_del(&cdev);
	device_destroy(dev_class, first);
	class_destroy(dev_class);
	unregister_chrdev_region(first,1);
	printk(KERN_ALERT "Device unregistered....\n");
}

static const struct of_device_id gpio_led_dt[] = {{.compatible = "sample-gpio-led", }, {}};

MODULE_DEVICE_TABLE(of, gpio_led_dt);
static struct platform_driver sample_pldriver = {
	.probe = sample_drv_probe,
	.remove = sample_drv_remove,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = of_match_ptr(gpio_led_dt)
	}
};

static int __init sample_init_gpio(void)
{
	printk(KERN_ALERT "sample gpio platform driver....\n");
	platform_driver_register(&sample_pldriver);
	return 0;
}

void __exit sample_cleanup_gpio(void)
{
	printk(KERN_ALERT "exiting the sample platform driver....\n");
	platform_driver_unregister(&sample_pldriver);
	return;
}

module_init(sample_init_gpio);
module_exit(sample_cleanup_gpio);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jay");
MODULE_DESCRIPTION("My first character Device Driver");
