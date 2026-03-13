#include <linux/kernel.h> // printk
#include <linux/module.h> // module_license, module_author, module_description
#include <linux/init.h> // module_init, module_exit
#include <linux/fs.h> // open, read, write, ioctl, release, struct file_operations, register_chrdev_region, unregister_chrdev_region, register_chrdev, alloc_chrdev_region
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/timer.h>

#define TIMEOUT 5000

static int __init timer_driver_init(void);
static void __exit timer_driver_exit(void);
void timer_callback(struct timer_list *timer);

unsigned int i = 0;

static struct timer_list chr_timer;

// timer callback function which is called when timer expires
void timer_callback(struct timer_list *timer)
{
	printk(KERN_INFO "in timer callback function[%d]\n", i++);
	
	// Re-enable timer which will make this timer as periodic timer
	mod_timer(&chr_timer, msecs_to_jiffies(TIMEOUT));
}

static int __init timer_driver_init(void)
{	
	// setup your timer to call timer callback function
	timer_setup(&chr_timer, timer_callback, 0);
	
	// setup the timer interval to base on TIMEOUT macro
	mod_timer(&chr_timer, jiffies + msecs_to_jiffies(TIMEOUT));

	printk(KERN_INFO "Device driver insert.... done properly...\n");
	return 0;
}

static void __exit timer_driver_exit(void)
{
	del_timer(&chr_timer);

	printk(KERN_INFO "Device driver is removed successfully....\n");
}

module_init(timer_driver_init);
module_exit(timer_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jay");
MODULE_DESCRIPTION("My first timer Driver");
