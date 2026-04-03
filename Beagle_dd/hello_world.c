#include <linux/module.h>
#include <linux/init.h>

/*Module's init entry point */
static int __init hello_world_init(void)
{
   pr_info("Hello world\n");
   return 0;
}

/*Module's cleanup entry point */
static void __exit hello_world_cleanup(void)
{
  pr_info("Good bye world\n");
}

module_init(hello_world_init);
module_exit(hello_world_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jay");
MODULE_DESCRIPTION("A simple hello world kernel module");
MODULE_INFO(board,"Beaglebone black REV A5");
