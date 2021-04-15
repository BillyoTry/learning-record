## 完善内核

### 调用约定

调用约定主要体现在以下三方面：

1. 参数的传递方式，参数是存放在寄存器中还是栈中
2. 参数的传递顺序，是从左到右传递还是从右到左传递
3. 是调用者保存寄存器环境还是被调用者保存

有如下常见的调用约定，我们主要关注cdecl、stdcall、thiscall即可

![image-20200819143424257](https://inews.gtimg.com/newsapp_ls/0/13412162903/0)

cdecl是默认c的调用约定，调用者将所有参数从右向左入栈，调用者清理参数所占栈空间，举个例子

```
int subtract(int a, int b); //被调用者  
int sub = subtract (3,2); // 主调用者 
```

调用者汇编如下

```
push 2
push 3
call subtract
add esp,8     ;回收(清理)栈空间
```

被调用者汇编如下

```
push ebp               ; 备份ebp
mov esp, ebp           ; esp赋值给ebp
mov eax, [ebp + 0x8]   ; 偏移8字节处为第一个参数a
add eax, [ebp + 0xc]   ; 偏移0xc字节处是第二个参数b
mov esp, ebp           ; 本句可有可无
pop ebp                ; 恢复ebp
ret                    ; 函数返回时esp+8,被调用函数清理栈中参数
```

stdcall是微软Win32 API的标准，调用者将所有参数从右向左入栈，并且调用者清理参数所占栈空间，还是上面的例子，调用者汇编如下

```
push 2
push 3
call subtract
```

被调用者汇编如下

```
push ebp               ; 备份ebp
mov esp, ebp           ; esp赋值给ebp
mov eax, [ebp + 0x8]   ; 偏移8字节处为第一个参数a
add eax, [ebp + 0xc]   ; 偏移0xc字节处是第二个参数b
mov esp, ebp           ; 本句可有可无
pop ebp                ; 恢复ebp
ret 8                  ; 函数返回时esp+8,被调用函数清理栈中参数
```

进入subtract函数时栈中的布局如下

![image-20200820012805009](https://inews.gtimg.com/newsapp_ls/0/13412163315/0)

thiscall则在C++中非静态成员函数的默认调用约定，其主要区别是ecx会多保存一个this指针指向操作的对象。

### 系统调用

为了更加理解系统调用，在后面会更频繁的结合C和汇编进行操作，下面做一个实验，分别用三种方式调用write函数，模拟下面C调用库函数的过程

```
#include<unistd.h>
int main(){
    write(1,"hello,world\n",4);
    return 0;
}
```

模拟代码`syscall_write.S`如下

```
section .data
str_c_lib: db "C library says: hello world!", 0xa ; 0xa为换行符
str_c_lib_len equ $-str_c_lib

str_syscall: db "syscall says: hello world!", 0xa
str_syscall_len equ $-str_syscall

section .text
global _start
_start:
; ssize_t write(int fd,const void *buf,size_t count);
; 方法一:模拟C语言中系统调用库函数write
	push str_c_lib_len
	push str_c_lib
	push 1
	
	call my_write
	add esp, 12
	
; 方法二:系统调用 此处传参顺序是ebx ecx edx esi edi
	mov eax, 4               ; 系统调用号
	mov ebx, 1               ; fd
	mov ecx, str_syscall     ; buf
	mov edx, str_syscall_len ; count
	int 0x80
	
; 退出程序
	mov eax, 1 ; exit()
	int 0x80

; 下面模拟write系统调用
my_write:
	push ebp
	mov esp, ebp
	mov eax, 4
	mov ebx, [ebp + 8]    ; fd
	mov ecx, [ebp + 0xc]  ; buf
	mov edx, [ebp + 0x10] ; count
	int 0x80
	pop ebp
	ret
```

运行结果如下：

![image-20200821212839344](https://inews.gtimg.com/newsapp_ls/0/13412163607/0)

既然我们用汇编模拟了C中的write函数，下面就用C结合汇编进行第二个实验

`C_with_S_c.c`

```
extern void asm_print(char*,int);
void c_print(char* str) {
    int len=0;
    while(str[len++]);   // 循环求出长度len,以'\0'结尾
    asm_print(str, len);
}
```

`C_with_S_S.S`

```
section .data
str: db "asm_print hello world!", 0xa, 0 ; 0xa为换行符,0为结束符
str_len equ $-str

section .text
extern c_print
global _start
_start:
	push str
	call c_print
	add esp, 4
	
; 退出程序
	mov eax, 1 ; exit()
	int 0x80

; 下面模拟write系统调用
global asm_print
asm_print:
	push ebp
	mov ebp, esp
	mov eax, 4
	mov ebx, 1
	mov ecx, [ebp + 8]   ; str
	mov edx, [ebp + 0xc] ; len
	int 0x80
	pop ebp
	ret
```

其调用关系如下图

![image-20200821213208808](https://inews.gtimg.com/newsapp_ls/0/13412163884/0)

编译输出`asm_print hello world!`。

### 实现打印函数

对于字符的打印主要是对显卡端口的操作，所以是用汇编实现，这里新键一个lib目录专门各种库文件，里面添加一个头文件，主要申请一些数据结构信息，来自Linux源码

```
#ifndef _LIB_STDINT_H_
#define _LIB_STDINT_H_

typedef signed char int8_t;
typedef signed short int int16_t;
typedef signed int int32_t;
typedef signed long long int int64_t;
typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long int uint64_t;

#endif //!_LIB_STDINT_H_
```

再在lib目录下新建一个user目录和一个kernel目录，我们的print实现代码就在kernel目录下的`print.S`，这个函数比较复杂，处理流程如下

1. 备份寄存器现场
2. 获取光标坐标值，光标坐标值是下一个可打印字符的位置
3. 获取待打印的字符
4. 判断字符是否为控制字符，如回车、换行、退格符需要特殊处理
5. 判断是否需要滚屏
6. 更新光标坐标值，使其指向下一个打印字符的位置
7. 恢复寄存器现场，退出

这里需要注意的是光标和字符的区别，他们之间没用任何关系，并不是“光标在哪字符就在哪”，这只是我们人为有意设置的。光标位置保存在光标寄存器中，可以手动维护，我们需要操作CRT控制数据寄存器中索引位 0x0E 的Cursor Location High Register和索引为 0x0F 的Cursor Location Low Register分别用来存储光标坐标的高8位和低八位。访问CRT寄存器，需要首先往端口地址为0x3D4寄存器写入索引，然后再从端口0x3D5的数据寄存器读写数据，另外一些特殊字符需要特殊处理，其中还会涉及到滚屏操作，我们的大屏幕是**80*25**大小的

1. 将第 1～24 行的内容整块搬到第 0～23 行，也就是把第 0 行的数据覆盖
2. 再将第 24 行，也就是后一行的字符用空格覆盖，这样它看上去是一个新的空行
3. 把光标移到第 24 行也就是后一行行首

```
TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0       

[bits 32]
section .text
; ----------------- put_char -----------------
; 把栈中的一个字符写入光标所在处
; --------------------------------------------
global put_char ; 全局变量，外部可调用
put_char:
	pushad ; 备份环境 入栈顺序为 eax ecx edx ebx esp ebp esi edi
	; 保证gs中为正确的视频段选择子
	; 为保险起见，每次打印时都为gs赋值
	mov ax, SELECTOR_VIDEO ; 不能直接把立即数送入段寄存器
	mov gs, ax
	
; 获取当前光标位置，25个字符一行，一共80行，从0行开始
; 先获得高8位 索引为0xEh
	mov dx, 0x03d4  ; 索引寄存器,存索引值
	mov al, 0x0e    ; 用于提供光标位置的高8位
	out dx, al
	mov dx, 0x03d5  ; 通过读写数据端口0x3d5来获得或设置光标位置,这里是操作用的寄存器
	in  al, dx      ; 得到了光标位置的高8位
	mov ah, al
	
	; 在获取低8位光标 索引为0xFh
	mov dx, 0x3d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x3d5
	in  al, dx
	; 将16位完整的光标存入bx
	mov bx, ax
	; 下面这行是在栈中获取待打印的字符
	mov ecx, [esp + 36] ; pushad压入4x8=32字节
						; 加上主函数4字节返回地址
	cmp cl, 0xd			; 回车CR是0x0d，换行LF是0x0a
	jz .is_carriage_return ; 回车与换行相同处理
	cmp cl, 0xa
	jz .is_line_feed       ; 换行与回车相同处理
	
	cmp cl, 0x8			; BS(backspace)的asc码是8
	jz .is_backspace
	jmp .put_other

.is_backspace:
	;;;;;;;;;;;;;;;;;; 对于backspace的一点说明 ;;;;;;;;;;;;;;;;;;
	; 当为 backspace 时，光标前移一位
	; 末尾添加空格或空字符0
	dec bx
	shl bx, 1				; 光标左移一位等于乘2
							; 表示光标对应显存中的偏移字节
	mov byte [gs:bx], 0x20	; 将待删除的字节补为0或空格皆可
	inc bx
	mov byte [gs:bx], 0x07
	shr bx, 1
	jmp .set_cursor
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.put_other:                  ; 处理可见字符
	shl bx, 1      			; 光标位置用2字节表示，将光标值乘2         
						    ; 表示对应显存中的偏移字节
	mov [gs:bx], cl         ; ASCII字符本身
	inc bx
	mov byte [gs:bx], 0x07  ; 字符属性
	shr bx, 1				; 恢复老的光标值
	inc bx					; 下一个光标值
	cmp bx, 2000
	jl .set_cursor			; 若光标值小于2000，表示未写到显存的最后，则去设置新的光标值
							; 若超出屏幕字符数大小(2000)则换行处理
.is_line_feed:				; 是换行符LF(\n)
.is_carriage_return:			; 是回车符
; 如果是CR(\r)，只要把光标移到行首就行了
	xor dx, dx 				; dx是被除数的高16位，清0
	mov ax, bx				; ax是被除数的低16位
	mov si, 80              ; 效访Linux中\n表示下一行的行首
	div si					; 这里\n和\r都处理为下一行的行首
	sub bx, dx				; 光标值减去除80的余数便是取整
							; 以上4行处理\r的代码
.is_carriage_return_end:    ; 回车符CR处理结束
	add bx, 80
	cmp bx, 2000
.is_line_feed_end:			; 若是LF(\n)，将光标移+80便可
	jl .set_cursor
; 屏幕行范围是0~24，滚屏的原理是将屏幕的第1~24行搬运到第0~23行，再将第24行用空格填充
.roll_screen:				; 若超出屏幕大小，开始滚屏
	cld
	mov ecx, 960			; 2000-80=1920个字符要搬运，共1920*2=3820字节
							; 一次搬4字节，共3840/4=960次
	mov esi, 0xc00b80a0		; 第一行行首
	mov edi, 0xc00b8000		; 第0行行首
	rep movsd
	
; 将最后一行填充为空白
	mov ebx, 3840			; 最后一行首字符的第一个字节偏移=1920*2
	mov ecx, 80				; 一行是80字符(160字节)，每次清空1字符(2字节)，一行需要移动80次
	
.cls:                         ;准备清空最后一行
	mov word [gs:ebx], 0x0720 	; 0x0720是黑底白字的空格键
	add ebx, 2                  ;循环处理每个字符(2字节)
	loop .cls
	mov bx, 1920			  	; 将光标值重置为1920，最后一行的首字符

.set_cursor:                     
; 将光标设为bx值
; 1.先设置高8位
	mov dx, 0x03d4			  	; 索引寄存器
	mov al, 0x0e				; 用于提供光标位置的高8位
	out dx, al
	mov dx, 0x03d5				; 通过读写数据端口0x3d5来获得或设置光标位置
	mov al, bh
	out dx, al
	
; 2.再设置低8位
	mov dx, 0x3d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x03d5
	mov al, bl
	out dx, al
.put_char_done:
	popad
	ret
```

头文件`print.h`

```
#ifndef __LIB_KERNEL_PRINT_H // 如果没有__LIB_KERNEL_PRINT_H宏则编译下面的代码
#define __LIB_KERNEL_PRINT_H
#include "stdint.h"
void put_char(uint8_t char_asci); // 这里是8位无符号整型,为了和之前参数存放在cl寄存器长度吻合
#endif
```

下面测试代码`main.c`

```
#include "print.h"
void main(void){
	put_char('k');
	put_char('e');
	put_char('r');
	put_char('n');
	put_char('e');
	put_char('l');
	put_char('\n');
	put_char('C');
	put_char('7');
	while(1);
}
```

编译`print.S`和`main.c`，然后链接`main.o`和`print.o`成`kernel.bin`文件，然后写入虚拟硬盘

```
sudo nasm -f elf -o print.o print.S
sudo gcc -m32 -I /home/qdl/bochs-2.6.2/bin/lib/kernel -c -o main.o main.c
sudo ld -m elf_i386 -Ttext 0xc0001500 -e main -o kernel.bin main.o /home/qdl/bochs-2.6.2/bin/lib/kernel/print.o
sudo dd if=./kernel.bin of=/home/qdl/bochs-2.6.2/bin/hd60M.img bs=512 count=200 seek=9 conv=notrunc
```

显示结果如下

![image-20200822093736318](https://inews.gtimg.com/newsapp_ls/0/13412164525/0)

下面把`put_char`函数封装起来，`put_str`通过`put_char`来打印以**0**字符结尾的字符串，思想就是循环打印直到遇到`\0`结束

```
; --------------------------------------------
; put_str通过put_char来打印以0字符结尾的字符串
; 输入：栈中参数为打印的字符串
; 输出：无
; --------------------------------------------
global put_str
put_str:
; 此函数用到ebx和ecx，先备份
	push ebx
	push ecx
	xor ecx, ecx
	mov ebx, [esp + 0xc] ; 栈中得到待打印字符串的地址
.goon:
	mov cl, [ebx]
	cmp cl, 0       ; 如果处理到了字符串尾，跳到结束处返回
	jz .str_over
	push ecx		; 为put_char函数传递参数
	call put_char	; 循环调用put_char实现打印字符串
	add esp, 4
	inc ebx			; ebx指向下一个字符
	jmp .goon

.str_over:
	pop ecx
	pop ebx
	ret
```

`print.h`中增加一行申明

```
#ifndef __LIB_KERNEL_PRINT_H // 如果没有__LIB_KERNEL_PRINT_H宏则编译下面的代码
#define __LIB_KERNEL_PRINT_H
#include "stdint.h"
void put_char(uint8_t char_asci); // 这里是8位无符号整型,为了和之前参数存放在cl寄存器长度吻合
void put_str(char* message);
#endif
```

`main.c`对其进行调用测试

```
#include "print.h"
void main(void){
	put_str("Welcome to C7 kernel\n");
	while(1);
}
```

测试结果如下

![image-20200822100113588](https://inews.gtimg.com/newsapp_ls/0/13412165540/0)

前面是实现对字符的打印，下面需要增加对整数的打印，逐位处理，A~F再单独处理，再增加对高位多余0的处理，详情见注释

```
;--------------------   将小端字节序的数字变成对应的ascii后，倒置   -----------------------
;输入：栈中参数为待打印的数字
;输出：在屏幕上打印16进制数字,并不会打印前缀0x,如打印10进制15时，只会直接打印f，不会是0xf
;------------------------------------------------------------------------------------------

global put_int
put_int:
	pushad
	mov ebp, esp
	mov eax, [ebp + 4*9] ; call的返回地址占4字节再加上pushad的8个四字节
	mov edx, eax
	mov edi, 7	; 指定在put_int_buffer中初始的偏移量	
	mov ecx, 8	; 32位数字中，十六进制数字的位数是8个
	mov ebx, put_int_buffer
	
; 将32位数字按照十六进制的形式从低位到高位逐个处理
; 共处理8个十六进制数字
.16based_4bits:	; 每4位二进制是16进制数字的1位
; 遍历每一位十六进制数字
	and edx, 0x0000000F	; 解析十六进制数字的每一位
						; and与操作后，edx只有低4位有效
	cmp edx, 9			; 数字0~9和a~f需要分别处理成对应的字符
	jg .is_A2F
	add edx, '0'		; ASCII码是8位大小。add求和操作后，edx低8位有效
	jmp .store
.is_A2F:
	sub edx, 10			; A~F减去10所得到的差，再加上字符A的
						; ASCII码，便是A~F对应的ASCII码
	add edx, 'A'
; 将每一位数字转换成对应的字符后,按照类似“大端”的顺序存储到缓冲区put_int_buffer
; 高位字符放在低地址,低位字符要放在高地址,这样和大端字节序类似,只不过咱们这里是字符序.
.store:
; 此时dl中应该是数字对应的字符的ASCII码
	mov [ebx + edi], dl
	dec edi
	shr eax, 4
	mov edx, eax
	loop .16based_4bits
	
; 现在put_int_buffer中已全是字符，打印之前
; 把高位连续的字符去掉，比如把字符000123变成123
.ready_to_print:
	inc edi		; 此时edi退减为-1(0xffffffff),加上1使其为0
.skip_prefix_0:
	cmp edi, 8	; 若已经比较第9个字符了
				; 表示待打印的字符串为全0
	je .full0
; 找出连续的0字符，edi作为非0的最高位字符的偏移
.go_on_skip:
	mov cl, [put_int_buffer + edi]
	inc edi
	cmp cl, '0'
	je .skip_prefix_0	; 继续判断下一位字符是否为字符0(不是数字0)
	dec edi				; edi在上面的inc操作中指向了下一个字符
	; 若当前字符不为'0'，要使edi减1恢复指向当前字符
	jmp .put_each_num
	
.full0:
	mov cl, '0'			; 输入的数字为全0时，则只打印0
.put_each_num:
	push ecx			; 此时cl中为可打印的字符
	call put_char
	add esp, 4
	inc edi				; 使edi指向下一个字符
	mov cl, [put_int_buffer + edi] ; 获取下一个字符到cl寄存器
	cmp edi, 8
	jl .put_each_num
	popad
	ret
```

在`print.h`增加一行`put_int`的申明注释，`main.c`中增加测试代码即可，测试结果如下所示

![image-20200822101914541](https://inews.gtimg.com/newsapp_ls/0/13412166035/0)

