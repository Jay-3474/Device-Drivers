#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <linux/ioctl.h>
#define GPIO_SELECT_LED _IOW('1', '1', int)

int main()
{
	int fd = -1;
	int option;
	int count=1;
	unsigned char c;

	fd = open("/dev/gpio_drv0", O_RDWR);
	if(fd == -1)
	{
		perror("led op_open");
	}

	do
	{
		printf("1. Getting the LED status...\n");
		printf("2. set the LED status...\n");
		printf("3. close...\n");
		printf("0. exit...\n");
		printf("enter your option\n");
		
		scanf("%d", &option);
		getchar();
		
		switch(option)
		{
			case 0:
				break;
			case 1:
				if(fd == -1)
				{
					perror("file not open");
					break;
				}
				count = read(fd, &c, 1);
				if(count == -1)
				{
					perror("led_ops read");
				}
				else
				{
					printf("led value is %d", c);
				}
				printf("\n");
				break;
			case 2:
				printf("enter your option....[0 - turn off, 1 - turn on]\n");
				c = getchar();
				count = write(fd, &c, 1);
				if(count == -1)
				{
					perror("led_ops write");
					break;
				}
				break;
			case 3:
				close(fd);
				break;
			default:
				printf("Invalid option...\n");
				break;
		}
	}while(option != 0);
	return 0;
}
