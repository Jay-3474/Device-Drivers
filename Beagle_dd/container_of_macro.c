/* container address is the referance address or first member address */
#include <stdio.h>

struct some_data
{
	char a; // four bytes with padding
	int  b; // four bytes
	char c; // four bytes with padding
	int  d; // four bytes
}; // total size is 16 bytes

struct some_data data;

#define container_of(ptr, type, member) ((type*)((char*)ptr - (char*)(&((type*)0)->member)))

void get_container(char *ptr)
{
	long offset;
	//struct some_data temp;
	
	//offset = (int)((char*)&temp.c - (char*)&temp.a);
	
	offset = (long)(&((struct some_data*)0)->c);
	struct some_data *data1 = (struct some_data*)(ptr - offset);
	
	printf("data.a = %d\n", data1->a);
	printf("data.b = %d\n", data1->b);
	printf("data.c = %c\n", data1->c);
	printf("data.d = %d\n", data1->d);
}

int main()
{
	data.a = 10;
	data.b = 5;
	data.c = 'a';
	data.d = 100;
	
	// passing members address
	//get_container(&data.c);
	
	struct some_data *data1 = container_of(&data.b, struct some_data, b);
	
	printf("data.a = %d\n", data1->a);
	printf("data.b = %d\n", data1->b);
	printf("data.c = %c\n", data1->c);
	printf("data.d = %d\n", data1->d);
	
	return 0;
}
