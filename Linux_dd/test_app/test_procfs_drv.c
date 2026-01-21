#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int8_t write_buf[1024];
int8_t read_buf[1024];

int main()
{
	int fd;
	char option;
	printf("demo of character device driver....\n");

	fd = open("/proc/chr_proc", O_RDWR);
	if(fd < 0)
	{
		printf("cannot open device file...\n");
		return 0;
	}
	while(1)
	{
		printf("*****enter options*****\n");
		printf("1. Write\n");
		printf("2. Read\n");
		printf("3. Exit\n");
		scanf(" %c", &option);
		printf("Your Options = %c\n", option);

		switch(option)
		{
			case '1':
				printf("Enter the string to write into the driver:\n");
				scanf(" %[^\t\n]s", write_buf);
				printf("Data written...\n");
				write(fd, write_buf, strlen(write_buf)+1);
				printf("DONE....\n");
				break;
			case '2':
				printf("Data id Reading...\n");
				read(fd, read_buf, 1024);
				printf("DONE....\n");
				printf("Data = %s\n\n", read_buf);
				break;
			case '3':
				close(fd);
				exit(1);
				break;
			default:
				printf("Enter valid option=%c\n", option);
		}
	}
	close(fd);
	return 0;
}
