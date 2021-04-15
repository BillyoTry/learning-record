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

; 方法二:系统调用
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