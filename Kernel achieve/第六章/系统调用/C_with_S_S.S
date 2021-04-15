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