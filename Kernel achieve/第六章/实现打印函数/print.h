#ifndef __LIB_KERNEL_PRINT_H // 如果没有__LIB_KERNEL_PRINT_H宏则编译下面的代码
#define __LIB_KERNEL_PRINT_H
#include "stdint.h"
void put_char(uint8_t char_asci); // 这里是8位无符号整型,为了和之前参数存放在cl寄存器长度吻合
void put_str(char* message);
void put_int(uint32_t num);  // 以十六进制打印
#endif