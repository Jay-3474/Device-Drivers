# Device-Drivers
Projects and informations related to Device Drivers

##Commands
- make
- make clean
- insmod hello_world.ko
- dmesg
- rmmod hello_world
- lsmod (Shows all loaded drivers)

##info
- my_device will be in /dev
- my_dev will be in /proc/devices
- my_custome_driver will be in /proc/modules
- use cc test_chr_drv.c -o test_chr_drv
- sudo ./test_chr_drv

##In-tree building
- If you want to add the linux module inside the linux kernel source tree and let the linux build system builds that. If you want to list your kernel module selection in kernel menuconfig,then create and use a Kconfig file.
- steps:
1.create a folder in linux-<version>/drivers/char/my_c_dev
2.copy main.c
3.create Kconfig file and add the below entries:
  menu "my custom menu"
  config CUSTOM_HELLOWORLD
          tristate "helloworld module support"
          default n
  endmenu
4.add the local Kconfig entry to upper(char directory) level Kconfig
  open Kconfig from char directory and go to the end, write source "drivers/char/my_c_dev/Kconfig"
5.create a local Makefile
  go to "drivers/char/my_c_dev" and create Makefile add the content in step 6
6.add obj-<config_item>+=<module>.o in the local Makefile
  obj-$(CONFIG_CUSTOM_HELLOWORLD) += main.o
7.add the local makefile to higher level makefile
  got to "drivers/char" and edit Makefile, go to the end and add
  obj-y += my_c_dev/
8.go to linux-<version> directory run the command
  make ARCH=arm menuconfig, it will open the menu
9.go to device drivers->character devices->my custom menu->helloworld module support
10.select * or M using space key and save
11.open .config file to verify that CONFIG_CUSTOM_HELLOWORD contains selected value
12.execute make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules -j4, you can see the module has build and .ko file is generated.
13.go to drivers/char/my_c_dev and run modinfo main.ko, you will find intree as Y
