#include "print.h"
#include "init.h"
void main(void){
    put_str("Welcome to C7's kernel\n");
    init_all();
    asm volatile("sti");
    while(1);
}