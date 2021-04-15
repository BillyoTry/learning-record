%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR ;初始化的栈顶
jmp loader_start

;构建gdt以及内部的描述符，每个8字节，由两个四字节组成
;第0个描述符不可用,置为0
GDT_BASE: dd 0x00000000 ;低四字节
          dd 0x00000000 ;高四字节
;代码段描述符
CODE_DESC: dd 0x0000FFFF      ;0xFFFF是段界限的0~15位，0x0000是段基址的0~15位
           DESC_CODE_HIGH4    ;boot.inc中定义的高四字节
;数据段和栈段描述符
DATA_STACK_DESC: dd 0x0000FFFF
                 dd DESC_DATA_HIGH4
;显存段描述符，为了方便显存操作，显存段不用平坦模型
VIDEO_DESC: dd 0x80000007        ;参考1MB实模式内存分布，limit=(0xbffff-0xb8000)/4k=0x7
            dd DESC_VIDEO_HIGH4  ;此时dpl为0
GDT_SIZE equ $ - GDT_BASE  ;地址差获得GDT大小
GDT_LIMIT equ GDT_SIZE - 1 ;大小减1获得段界限
times 60 dq 0 ;此处预留60个描述符空位，为以后做准备，times相当于是循环执行命令
;构建代码段、数据段、显存段的选择子
SELECTOR_CODE equ (0x0001<<3)+TI_GDT+RPL0	;相当于（CODE_DESC-GDT_BASE）/8+TI_GDT+RPL0
SELECTOR_DATA equ (0x0002<<3)+TI_GDT+RPL0
SELECTOR_VIDEO equ (0x0003<<3)+TI_GDT+RPL0

;以下是gdt的指针，前2字节也就是16位是gdt界限，后4字节也就是32位是gdt起始地址
gdt_ptr dw GDT_LIMIT
        dd GDT_BASE
loadermsg db '2 loader in real.'    ;还是在实模式下打印 的，用的还是 BIOS 中断，
loader_start:
;---------------------------------------------------------
;INT 0x10	功能号:0x13	功能描述符:打印字符串
;---------------------------------------------------------
;输入:
;AH 子功能号=13H
;BH = 页码
;BL = 属性（若AL=00H或01H）
;CX = 字符串长度
;(DH,DL)=坐标(行，列)
;ES:BP=字符串地址
;AL=显示输出方式
;0——字符串中只含显示字符，其显示属性在BL中。显示后，光标位置不变
;1——字符串中只含显示字符，其显示属性在BL中。显示后，光标位置改变
;2——字符串中只含显示字符和显示属性。显示后，光标位置不变。
;3——字符串中只含显示字符和显示属性。显示后，光标位置改变。
;无返回值
mov sp,LOADER_BASE_ADDR
mov bp,loadermsg		;ES:BP=字符串地址
mov cx,17				;CX=字符串长度
mov ax,0x1301			;AH=13,AL=01h
mov bx,0x001f			;页号为0(BH=0)蓝底分红子(BL=1fh)
mov dx,0x1800
int 0x10

;---------------------准备进入保护模式-------------------------
;1 打开A20
;2 加载gdt
;3 将cr0的pe位置1

;-----------------------打开A20--------------------------
in al,0x92
or al,0000_0010B
out 0x92,al
;-----------------------加载GDT--------------------------
lgdt [gdt_ptr]

;----------------------cr0 第 0 位置 1-------------------
mov eax,cr0
or eax,0x00000001
mov cr0,eax

jmp dword SELECTOR_CODE:p_mode_start		;下面指令又有16位又有32位，故需要刷新流水线

[bits 32]
p_mode_start:
;选择子初始化段寄存器
	mov ax,SELECTOR_DATA
	mov ds,ax
	mov es,ax
	mov ss,ax
	mov esp,LOADER_STACK_TOP
	mov ax,SELECTOR_VIDEO
	mov gs,ax
	
	mov byte [gs:160],'P' ;显存第80个字符的位置写一个P
	
	jmp $