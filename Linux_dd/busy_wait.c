#include <linux/kernel.h> // printk
#include <linux/module.h> // module_license, module_author, module_description
#include <linux/init.h> // module_init, module_exit
#include <linux/fs.h> // open, read, write, ioctl, release, struct file_operations, register_chrdev_region, unregister_chrdev_region, register_chrdev, alloc_chrdev_region
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/device.h>

int delay = HZ;

static struct task_struct *thread_wait;

static int __init busy_wait_driver_init(void);
static void __exit busy_wait_driver_exit(void);

static int thread_func(void *wait)
{
	unsigned long j1, j2;

	while(!kthread_should_stop())
	{
	j1 = jiffies;
	j2 = j1 + delay;

	while(time_before(jiffies, j2))
	{
		cpu_relax();
	}
	j2 = jiffies;
	printk(KERN_INFO "jiffies start = %lu\t and jiffies end = %lu\n", j1, j2);
	if(kthread_should_stop())
	{
		break;
	}
	}
	printk(KERN_INFO "Thread is stopped...\n");
	return 0;
}

static int __init busy_wait_driver_init(void)
{
	printk(KERN_INFO "Creating thread...\n");
	thread_wait = kthread_run(thread_func, NULL, "mythread");
	return 0;
}

static void __exit busy_wait_driver_exit(void)
{
	printk(KERN_INFO "Removing the module...\n");
	if(thread_wait != NULL)
	{
		kthread_stop(thread_wait);
		printk(KERN_INFO "stopping the thread...\n");
	}
	printk(KERN_INFO "Device driver is removed successfully....\n");
}

module_init(busy_wait_driver_init);
module_exit(busy_wait_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jay");
MODULE_DESCRIPTION("My first busy_wait Device Driver");
