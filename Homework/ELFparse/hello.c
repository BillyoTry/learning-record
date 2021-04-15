#include <stdio.h>

int main(int argc,char *argv[]){
	char* string = "hello world";
	printf("%s",string);
	puts("hello world");
	write(1,string,12);
}