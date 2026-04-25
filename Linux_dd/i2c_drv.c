#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/fs.h>

struct i2c_data {
	struct i2c_client* client;
	dev_t dev;
	u8* buf;
	u16 value;
	struct cdev cdev;
	struct class* class;
};

static int my_open(struct inode* inode, struct file* file)
{
	struct i2c_data* dev = container_of(inode->i_cdev, struct i2c_data, cdev);
	if(dev == NULL)
	{
		printk(KERN_ALERT "There is no data...\n");
		return -1;
	}
	file->private_data = dev;
	return 0;
}

static int my_close(struct inode* inode, struct file* file)
{
	return 0;
}

static ssize_t my_write(struct file* file, const char* buf, size_t count, loff_t* off)
{
	struct i2c_data* dev = (struct i2c_data*)(file->private_data);
	struct i2c_client* client = dev->client;
	struct i2c_adapter* adap = client->adapter;
	struct i2c_msg msg;
	
	char *temp;
	temp = memdup_user(buf, count);
	msg.addr = 0x68;
	msg.flags = 0;
	msg.len = count;
	msg.buf = temp;
	
	int ret;
	ret = i2c_transfer(adap, &msg, 1);
	kfree(temp);
	return (ret == 1? count: ret);
}

static ssize_t my_read(struct file* file, char* buf, size_t count, loff_t* off)
{
	struct i2c_data* dev = (struct i2c_data*)(file->private_data);
	struct i2c_client* client = dev->client;
	struct i2c_adapter* adap = client->adapter;
	struct i2c_msg msg;
	
	char *temp;
	temp = kmalloc(count, GFP_KERNEL);
	
	msg.addr = 0x68;
	msg.flags = 0;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.buf = temp;
	
	int ret;
	ret = i2c_transfer(adap, &msg, 1);
	
	if(ret >= 0)
	ret = copy_to_user(buf, temp, count) ? -EFAULT : count;
	kfree(temp);
	return ret;
}

struct file_operations fops = {
	.open = my_open,
	.release = my_close,
	.read = my_read,
	.write = my_write
};

static void ds3231_remove(struct i2c_client* client)
{
	struct i2c_data* data;
	printk(KERN_INFO "Remove function is invoked..\n");
	data = i2c_get_clientdata(client);
	cdev_del(&data->cdev);
	device_destroy(data->class, data->dev);
	class_destroy(data->class);
	unregister_chrdev_region(data->dev, 1);
	//return 0;
}

static int ds3231_probe(struct i2c_client* client)
{
	struct i2c_data* data;
	int res;
	
	printk(KERN_INFO "probe function is invoked...\n");
	data = devm_kzalloc(&client->dev, sizeof(struct i2c_data), GFP_KERNEL);
	
	data->value = 30;
	data->buf = devm_kzalloc(&client->dev, data->value, GFP_KERNEL);
	i2c_set_clientdata(client, data);
	
	res = alloc_chrdev_region(&data->dev, 0, 1, "i2c_dev");
	if(res < 0)
	{
		printk(KERN_ALERT "Device registaration unable...\n");
		unregister_chrdev_region(data->dev, 1);
		return -1;
	}
	printk(KERN_INFO "Major Number = %d\n", MAJOR(data->dev));
	
	if((data->class = class_create("i2cdriver")) == NULL)
	{
		printk(KERN_ALERT "Unable to create the device class..\n");
		unregister_chrdev_region(data->dev, 1);
		return -1;
	}
	
	if(device_create(data->class, NULL, data->dev, NULL, "i2c_drv%d", 0) == NULL)
	{
		printk(KERN_ALERT "Unable to create the device......\n");
		class_destroy(data->class);
		unregister_chrdev_region(data->dev, 1);
		return -1;
	}
	cdev_init(&data->cdev, &fops);
	
	if(cdev_add(&data->cdev, data->dev, 1) == -1)
	{
		printk(KERN_ALERT "Unable to add the device..\n");
		device_destroy(data->class, data->dev);
		class_destroy(data->class);
		unregister_chrdev_region(data->dev, 1);
		return -1;
	}
	return 0;
}



static const struct i2c_device_id i2c_ids[] = {
	{"ds3231", 0},
	{"ds32", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, i2c_ids);

static struct i2c_driver ds3231_I2C_drv = {
	.driver = {.name = "ds32", .owner = THIS_MODULE},
	.probe = ds3231_probe,
	.remove = ds3231_remove,
	.id_table = i2c_ids
};

static int __init i2c_client_drv_init(void)
{
	// Register with i2c-core
	i2c_add_driver(&ds3231_I2C_drv);
	return 0;
}

static void __exit i2c_client_drv_exit(void)
{
	i2c_del_driver(&ds3231_I2C_drv);
}

module_init(i2c_client_drv_init);
module_exit(i2c_client_drv_exit);

MODULE_DESCRIPTION("i2c client driver");
MODULE_AUTHOR("Jay");
MODULE_LICENSE("GPL");
