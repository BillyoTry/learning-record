## 保护模式进阶

### 获取物理内存容量

为了在后期做好内存管理工作，我们先得知道自己有多少物理内存才行.Linux获取内存容量方法有三种，本质上分别是BIOS中断0x15的3个子功能，BIOS是实模式下的方法，只能在保护模式之前调用。

**利用BIOS中断0x15子功能0xe820获取内存**

此方法最灵活，返回的内容也最丰富，内存信息的内容是地址范围描述符来描述的(ARDS)，每个字段4字节，一共20字节，调用0x15返回的也就是这个结构。其中Type字段表示内存类型，1表示这段内存可以使用；2表示不可用使用此内存；其它表示未定义，将来会用到。

![image-20200817001402399](https://inews.gtimg.com/newsapp_ls/0/13412146860/0)

此结构中的字段大小都是4字节，共五个字段，所以此结构位20字节大小。Type 字段的具体意义见表 5-2。 

![image-20200817001706229](https://inews.gtimg.com/newsapp_ls/0/13412147318/0)

用0x15子功能0xe820调用说明和调用步骤如下

1. 填写好"调用前输入"中列出的寄存器
2. 执行中断调用 int 0x15
3. 在CF位为0的情况下，"返回后输出"中对应的寄存器中就有结果

![image-20200817002022029](https://inews.gtimg.com/newsapp_ls/0/13412147133/0)

![image-20200817002030369](https://inews.gtimg.com/newsapp_ls/0/13412147774/0)

**利用BIOS中断0x15子功能0xe801获取内存**

此方法最多识别4G的内存，结果存放在两组寄存器中，操作起来要简便一些，调用说明和调用步骤如下

1. AX寄存器写入0xE801
2. 执行中断调用 int 0x15
3. 在CF位为0的情况下，"返回后输出"中对应的寄存器中就有结果

![image-20200817002549422](https://inews.gtimg.com/newsapp_ls/0/13412148454/0)

**利用BIOS中断0x15子功能0x88获取内存**

此方法最多识别64MB内存，操作起来最简单，调用说明和调用步骤如下

1. AX寄存器写入0x88
2. 执行中断调用 int 0x15
3. 在CF位为0的情况下，"返回后输出"中对应的寄存器中就有结果

![image-20200817002655680](https://inews.gtimg.com/newsapp_ls/0/13412149072/0)

下面结合这三种方式改进我们的实验代码，下面是`loader`，我们将结果保存在了`total_mem_bytes`中，重要的一些地方都有注释，更详细的内容建议参考原书。

```
%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR    ;0x900
LOADER_STACK_TOP equ LOADER_BASE_ADDR

;构建gdt及其内部的描述符
   GDT_BASE:   dd    0x00000000 
	       	   dd    0x00000000

   CODE_DESC:  dd    0x0000FFFF 
	           dd    DESC_CODE_HIGH4

   DATA_STACK_DESC:  dd    0x0000FFFF
		            dd    DESC_DATA_HIGH4

   VIDEO_DESC: dd    0x80000007	   ; limit=(0xbffff-0xb8000)/4k=0x7
	           dd    DESC_VIDEO_HIGH4  ; 此时dpl为0

   GDT_SIZE   equ   $ - GDT_BASE
   GDT_LIMIT   equ   GDT_SIZE -	1 
   times 60 dq 0					 ; 此处预留60个描述符的空位(slot)
   SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0     ; 相当于(CODE_DESC - GDT_BASE)/8 + 													 ;				TI_GDT + RPL0
   SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0	 ; 同上
   SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0	 ; 同上 
   
   ; total_mem_bytes用于保存内存容量,以字节为单位,此位置比较好记。
   ; (4个段描述符 + 60个段描述符槽位) * 8字节 = total_mem_bytes_offset = 0x200
   ; 当前偏移loader.bin文件头0x200字节,loader.bin的加载地址是0x900,
   ; 故total_mem_bytes内存中的地址是0x900+0x200=0xb00.将来在内核中咱们会引用此地址
   total_mem_bytes dd 0
   
   ;以下是定义gdt的指针，前2字节是gdt界限，后4字节是gdt起始地址
   gdt_ptr  dw  GDT_LIMIT
	        dd  GDT_BASE
	        
   ;人工对齐:total_mem_bytes4字节+gdt_ptr6字节+ards_buf244字节+ards_nr2,共256字节
   ards_buf times 244 db 0
   ards_nr dw 0		      ;用于记录ards结构体数量
   
   ;-------  int 15h eax = 0000E820h ,edx = 534D4150h ('SMAP') 获取内存布局  -------

   xor ebx, ebx		      ;第一次调用时，ebx值要为0
   mov edx, 0x534d4150	  ;edx只赋值一次，循环体中不会改变
   mov di, ards_buf	      ;ards结构缓冲区
.e820_mem_get_loop:	      ;循环获取每个ARDS内存范围描述结构
   mov eax, 0x0000e820	  ;执行int 0x15后,eax值变为0x534d4150,所以每次执行int前都要更新为子功能号。
   mov ecx, 20		      ;ARDS地址范围描述符结构大小是20字节
   int 0x15
   jc .e820_failed_so_try_e801   ;若cf位为1则有错误发生，尝试0xe801子功能
   add di, cx		      ;使di增加20字节指向缓冲区中新的ARDS结构位置
   inc word [ards_nr]	  ;记录ARDS数量
   cmp ebx, 0		      ;若ebx为0且cf不为1,这说明ards全部返回，当前已是最后一个
   jnz .e820_mem_get_loop
   
   
;在所有ards结构中，找出(base_add_low + length_low)的最大值，即内存的容量。
   mov cx, [ards_nr]	  ;遍历每一个ARDS结构体,循环次数是ARDS的数量
   mov ebx, ards_buf 
   xor edx, edx		      ;edx为最大的内存容量,在此先清0
.find_max_mem_area:	      ;无须判断type是否为1,最大的内存块一定是可被使用
   mov eax, [ebx]	      ;base_add_low
   add eax, [ebx+8]	      ;length_low
   add ebx, 20		      ;指向缓冲区中下一个ARDS结构
   cmp edx, eax		      ;冒泡排序，找出最大,edx寄存器始终是最大的内存容量
   jge .next_ards
   mov edx, eax		      ;edx为总内存大小
.next_ards:
   loop .find_max_mem_area
   jmp .mem_get_ok

;------  int 15h ax = E801h 获取内存大小,最大支持4G  ------
; 返回后, ax cx 值一样,以KB为单位,bx dx值一样,以64KB为单位
; 在ax和cx寄存器中为低16M,在bx和dx寄存器中为16MB到4G。
.e820_failed_so_try_e801:
   mov ax,0xe801
   int 0x15
   jc .e801_failed_so_try88   ;若当前e801方法失败,就尝试0x88方法

;1 先算出低15M的内存,ax和cx中是以KB为单位的内存数量,将其转换为以byte为单位
   mov cx,0x400	     ;cx和ax值一样,cx用做乘数
   mul cx 
   shl edx,16
   and eax,0x0000FFFF
   or edx,eax
   add edx, 0x100000 ;ax只是15MB,故要加1MB
   mov esi,edx	     ;先把低15MB的内存容量存入esi寄存器备份

;2 再将16MB以上的内存转换为byte为单位,寄存器bx和dx中是以64KB为单位的内存数量
   xor eax,eax
   mov ax,bx		
   mov ecx, 0x10000	;0x10000十进制为64KB
   mul ecx		    ;32位乘法,默认的被乘数是eax,积为64位,高32位存入edx,低32位存入eax.
   add esi,eax		;由于此方法只能测出4G以内的内存,故32位eax足够了,edx肯定为0,只加eax便可
   mov edx,esi		;edx为总内存大小
   jmp .mem_get_ok

;-----------------  int 15h ah = 0x88 获取内存大小,只能获取64M之内  ----------
.e801_failed_so_try88: 
   ;int 15后，ax存入的是以kb为单位的内存容量
   mov  ah, 0x88
   int  0x15
   jc .error_hlt
   and eax,0x0000FFFF
      
   ;16位乘法，被乘数是ax,积为32位.积的高16位在dx中，积的低16位在ax中
   mov cx, 0x400     ;0x400等于1024,将ax中的内存容量换为以byte为单位
   mul cx
   shl edx, 16	     ;把dx移到高16位
   or edx, eax	     ;把积的低16位组合到edx,为32位的积
   add edx,0x100000  ;0x88子功能只会返回1MB以上的内存,故实际内存大小要加上1MB

.mem_get_ok:
   mov [total_mem_bytes], edx	 ;将内存换为byte单位后存入total_mem_bytes处。


;-----------------   准备进入保护模式   -------------------
;1 打开A20
;2 加载gdt
;3 将cr0的pe位置1

   ;-----------------  打开A20  ----------------
   in al,0x92
   or al,0000_0010B
   out 0x92,al

   ;-----------------  加载GDT  ----------------
   lgdt [gdt_ptr]

   ;-----------------  cr0第0位置1  ----------------
   mov eax, cr0
   or eax, 0x00000001
   mov cr0, eax

   jmp dword SELECTOR_CODE:p_mode_start	   ; 刷新流水线，避免分支预测的影响,这种cpu优化策略，最怕jmp跳转，
					                     ; 这将导致之前做的预测失效，从而起到了刷新的作用。
.error_hlt:		      ;出错则挂起
   hlt

[bits 32]
p_mode_start:
   mov ax, SELECTOR_DATA
   mov ds, ax
   mov es, ax
   mov ss, ax
   mov esp,LOADER_STACK_TOP
   mov ax, SELECTOR_VIDEO
   mov gs, ax

   mov byte [gs:160], 'P'

   jmp $
```

在`mbr.S`中也需要修改一处内容，我们跳转的内容要加上0x300，原因是在 loader.S 中`loader_start`计算如下

> total_mem_bytes + gdt_ptr + ards_buf + adrs_nr + total_mem_bytes_offset = loader_start
>
> 4 + 6 + 244 + 2 + 0x200 = 0x300

在检测前我们可以看下我们的机器上到底装了多少内存，以此来验证我们的检测结果的正确性

![image-20200818004601931](https://inews.gtimg.com/newsapp_ls/0/13412149869/0)

运行结果如下，这里我们用xp 0xb00查看我们的结果，0x02000000换算过来刚好是我们bochsrc.disk中 megs 设置的32MB大小。

![image-20200818012854988](https://inews.gtimg.com/newsapp_ls/0/13412150313/0)

### 启动分页机制

分页机制是当物理内存不足时，或者内存碎片过多无法容纳新进程等情况的一种应对措施。假如说此时未开启分页功能，而物理内存空间又不足，如下图所示，此时线性地址和物理地址一一对应，没有满足进程C的内存大小，可以选择等待进程B或者A执行完获得连续的内存空间，也可以将A3或者B1段换到硬盘上，腾出一部分空间，然而这些IO操作过多会使机器响应速度很慢，用户体验很差。

![image-20200818013208816](https://inews.gtimg.com/newsapp_ls/0/13412150661/0)

出现这种情况的本质其实是在分段机制下，**线性地址等价于物理地址**。那么即使在进程B的下面还有10M的可用空间，但因为两块可用空间并不连续，所以进程C无法使用进程B下面的10M可用空间。

按照这种思路，只需要通过某种映射关系，**将线性地址映射到任意的物理地址**，就可以解决这种问题了。实现**线性地址的连续**，而**物理地址不需要连续**，于是分页机制就诞生了。

### 一级页表

在保护模式下寻址依旧是通过`段基址:段内偏移`组成的线性地址，计算出线性地址后再通过判断分页位是否打开，若打开则开启分页机制进行检索，如下图所示

![image-20200818013710764](https://inews.gtimg.com/newsapp_ls/0/13412151102/0)

分页机制的作用有

- 将线性地址转换成物理地址
- 用大小相等的页代替大小不等的段

分页机制的作用如下图所示，分页机制来映射的线性地址便是我们经常说的虚拟地址

![image-20200818013809190](https://inews.gtimg.com/newsapp_ls/0/13412151360/0)

因为`页大小 * 页数量 = 4GB`，想要减少页表的大小，只能增加一页的大小。最终通过数学求极限，定下**4KB为最佳页大小**。页表将线性地址转换成物理地址的过程总结如下图，首先通过计算线性地址高20位索引出页表中的基址，然后加上低12位计算出最终的物理地址，下图中0x9234即是最终的物理地址。

![image-20200818013928157](https://inews.gtimg.com/newsapp_ls/0/13412151857/0)

### 二级页表

无论是几级页表，标准页的尺寸都是4KB。所以4GB的线性地址空间最多有1M个标准页。一级页表是将这1M个标准页放置到一张页表中，二级页表是将这1M个标准页平均放置1K个页表中，每个页表包含有1K个页表项。页表项是4字节大小，页表包含1K个页表项，故页表的大小同样为4KB，刚好为一页。

为了管理页表的物理地址，专门有一个页目录表来存放这些页表。页目录表中存储的页表称为页目录项(PDE)，页目录项同样为4KB，且最多有1K个页目录项，所以页目录表也是4KB，如下图所示

![image-20200818214259009](https://inews.gtimg.com/newsapp_ls/0/13412152226/0)

二级页表中虚拟地址到物理地址的转换也有很大的变化，具体步骤如下

- 用虚拟地址的**高 10 位**乘以 4，作为页目录表内的偏移地址，加上页目录表的物理地址，所得的和，便是页目录项的物理地址。读取该页目录项，从中获取到页表的物理地址。
- 用虚拟地址的**中间 10 位**乘以 4，作为页表内的偏移地址，加上在第 1 步中得到的页表物理地址，所得的和，便是页表项的物理地址。读取该页表项，从中获取到分配的物理页地址。
- 虚拟地址的高 10 位和中间 10 位分别是 PDE和PIE 的索引值，所以它们需要乘以 4。但**低 12 位**就不是索引值了，其表示的范围是 0~0xfff，作为页内偏移最合适，所以虚拟地址的低 12 位加上第二步中得到的物理页地址，所得的和便是最终转换的物理地址。

下图表示`mov ax, [0x1234567]`的转换过程，可以发现cr3寄存器其实指向的是页目录表基地址

![image-20200818214502494](https://inews.gtimg.com/newsapp_ls/0/13412152720/0)

页目录表项PDE和页表项PTE的结构如下图所示

![image-20200818215133868](https://inews.gtimg.com/newsapp_ls/0/13412153089/0)

从右到左是各属性总结如下

- P，Present，意为存在位置。若为1表示该页存在于物理内存中，若为0表示该表不在物理内存中。
- RW，Read/Write，意为读写位。若为1表示可读可写，若为0表示可读不可写。
- US，User/Supervisor，意为普通用户/超级用户位。若为1，表示处于User级，任意级别(0、1、2、3)特权的程序都可以访问该页。若为0，表示处于Supervisor级，特权级别为3的程序不允许访问该页，只有特权级别为0、1、2的程序可以访问。
- PWT，Page-level Write-Through，意为页级通写位，若为 1 表示此项采用通写方式， 表示该页不仅是普通内存，还是高速缓存。
- PCD，Page-level Cache Disable，意为页级高速缓存禁止位。若为 1 表示该页启用高速缓存，为 0 表 示禁止将该页缓存。这里咱们将其置为 0。
- A，Accessed，意为访问位。
- D，Dirty，意为脏页位，当 CPU 对一个页面执行写操作时，就会设置对应页表项的 D 位为 1。此项 仅针对页表项有效，并不会修改页目录项中的 D 位。
- PAT，Page Attribute Table，意为页属性表位，能够在页面一级的粒度上设置内存属性。
- G,Global，意为全局位，为1表示该页在高速缓存TLB中一直保存。
- AVL，意为 Available 位，表示软件，系统可用该位，和CPU无关。

总结这些步骤，我们启用分页机制需要做的事情如下

1. 准备好页目录表及页表
2. 将页表地址写入控制寄存器cr3
3. 寄存器cr0的PG位置1

下面是第一步创建页目录及页表的代码

```
; 创建页目录及页表
setup_page:
; 先把页目录占用的空间逐字节清零
	mov ecx, 4096        ;4KB*1024  每一项4字节，总共1024项
	mov esi, 0
.clear_page_dir:
	mov byte [PAGE_DIR_TABLE_POS + esi], 0   ;PAGE_DIR_TABLE_POS为0x100000，这是出了低端1MB的                                               第一个字节,这也是页目录表的位置
	inc esi
	loop .clear_page_dir

; 开始创建页目录项(PDE)
.create_pde:        ; 创建PDE
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x1000 ; 此时eax为第一个页表的位置及属性，因为页目录表大小为4B*1024=4KB=0x1000
	mov ebx, eax    ; 此处为ebx赋值,是为.create_pte做准备,ebx为基址

; 下面将页目录项0和0xc00都存为第一个页表的地址，每个页表表示4MB内存
; 这样0xc03fffff以下的地址和0x003fffff以下的地址都指向相同的页表
; 这是为将地址映射为内核地址做准备
	or eax, PG_US_U | PG_RW_W | PG_P      ; 页目录项的属性RW和P位为1,US为1,表示用户属性,所有特										权级别都可以访问.
	mov [PAGE_DIR_TABLE_POS + 0x0], eax   ; 第1个目录项,在页目录表中的第1个目录项写入第一个页表的位置(0x101000)及属性(7)
	mov [PAGE_DIR_TABLE_POS + 0xc00], eax ; 一个页表项占用四字节
	; 0xc00表示第768个页表占用的目录项,0xc00以上的目录项用于内核空间
	; 也就是页表的0xc0000000~0xffffffff这1G属于内核
	; 0x0~0xbfffffff这3G属于用户进程
	sub eax, 0x1000
	mov [PAGE_DIR_TABLE_POS + 4092], eax  ; 使最后一个目录项指向页目录表自己的地址
	
; 下面创建页表项(PTE)
	mov ecx, 256	; 1M低端内存 / 每页大小 4K = 256
	mov esi, 0
	mov edx, PG_US_U | PG_RW_W | PG_P	; 属性为7
.create_pte:		; 创建PTE
	mov [ebx+esi*4], edx ; 此时的ebx为0x101000,也就是第一个页表的地址
	add edx, 4096
	inc esi
	loop .create_pte
	
; 创建内核其他页表的PDE
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x2000						; 此时eax为第二个页表的位置0x102000
	or eax, PG_US_U | PG_RW_W | PG_P	; 属性为7
	mov ebx, PAGE_DIR_TABLE_POS
	mov ecx, 254						; 范围为第769~1022的所有目录项数量,此处为内核使用部分
	mov esi, 769
.create_kernel_pde:
	mov [ebx+esi*4], eax                 ;mov 0x100000+769*4，102007
	inc esi
	add eax, 0x1000						;此处更新页表的物理地址，讲其写入页目录下中
	loop .create_kernel_pde 
	ret
```

在boot.inc中**添加**如下信息

```
; loader 和 kernel
PAGE_DIR_TABLE_POS equ 0x100000
; 页表相关属性
PG_P equ 1b
PG_RW_R equ 00b
PG_RW_W equ 10b
PG_US_S equ 000b
PG_US_U equ 100b
```

进行完第一步的内容，之后的操作相对就简单了，将第二步页表地址写入控制寄存器cr3寄存器和第三步将cr0的PG位置1的操作整合起来的`loader.S`如下所示

```
%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR

;构建gdt及其内部的描述符
GDT_BASE:   dd    0x00000000 
       		dd    0x00000000

CODE_DESC:  dd    0x0000FFFF 
       		dd    DESC_CODE_HIGH4

DATA_STACK_DESC:  dd    0x0000FFFF
	     		 dd    DESC_DATA_HIGH4

VIDEO_DESC: dd    0x80000007	   ; limit=(0xbffff-0xb8000)/4k=0x7
       		dd    DESC_VIDEO_HIGH4  ; 此时dpl为0

GDT_SIZE   equ   $ - GDT_BASE
GDT_LIMIT   equ   GDT_SIZE -	1 
times 60 dq 0					 ; 此处预留60个描述符的空位(slot)
SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0     ; 相当于(CODE_DESC - GDT_BASE)/8 + TI_GDT + RPL0
SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0	 ; 同上
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0	 ; 同上 

; total_mem_bytes用于保存内存容量,以字节为单位,此位置比较好记。
; 当前偏移loader.bin文件头0x200字节,loader.bin的加载地址是0x900,
; 故total_mem_bytes内存中的地址是0xb00.将来在内核中咱们会引用此地址
total_mem_bytes dd 0					 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;以下是定义gdt的指针，前2字节是gdt界限，后4字节是gdt起始地址
gdt_ptr  dw  GDT_LIMIT 
    dd  GDT_BASE

;人工对齐:total_mem_bytes4字节+gdt_ptr6字节+ards_buf244字节+ards_nr2,共256字节
ards_buf times 244 db 0
ards_nr dw 0		      ;用于记录ards结构体数量

loader_start:

;-------  int 15h eax = 0000E820h ,edx = 534D4150h ('SMAP') 获取内存布局  -------

   xor ebx, ebx		      ;第一次调用时，ebx值要为0
   mov edx, 0x534d4150	  ;edx只赋值一次，循环体中不会改变
   mov di, ards_buf	      ;ards结构缓冲区
.e820_mem_get_loop:	      ;循环获取每个ARDS内存范围描述结构
   mov eax, 0x0000e820	  ;执行int 0x15后,eax值变为0x534d4150,所以每次执行int前都要更新为子功能号。
   mov ecx, 20		      ;ARDS地址范围描述符结构大小是20字节
   int 0x15
   jc .e820_failed_so_try_e801   ;若cf位为1则有错误发生，尝试0xe801子功能
   add di, cx		      ;使di增加20字节指向缓冲区中新的ARDS结构位置
   inc word [ards_nr]	  ;记录ARDS数量
   cmp ebx, 0		      ;若ebx为0且cf不为1,这说明ards全部返回，当前已是最后一个
   jnz .e820_mem_get_loop

;在所有ards结构中，找出(base_add_low + length_low)的最大值，即内存的容量。
   mov cx, [ards_nr]	  ;遍历每一个ARDS结构体,循环次数是ARDS的数量
   mov ebx, ards_buf 
   xor edx, edx		      ;edx为最大的内存容量,在此先清0
.find_max_mem_area:	      ;无须判断type是否为1,最大的内存块一定是可被使用
   mov eax, [ebx]	      ;base_add_low
   add eax, [ebx+8]	      ;length_low
   add ebx, 20		      ;指向缓冲区中下一个ARDS结构
   cmp edx, eax		      ;冒泡排序，找出最大,edx寄存器始终是最大的内存容量
   jge .next_ards
   mov edx, eax		      ;edx为总内存大小
.next_ards:
   loop .find_max_mem_area
   jmp .mem_get_ok

;------  int 15h ax = E801h 获取内存大小,最大支持4G  ------
; 返回后, ax cx 值一样,以KB为单位,bx dx值一样,以64KB为单位
; 在ax和cx寄存器中为低16M,在bx和dx寄存器中为16MB到4G。
.e820_failed_so_try_e801:
   mov ax,0xe801
   int 0x15
   jc .e801_failed_so_try88   ;若当前e801方法失败,就尝试0x88方法

;1 先算出低15M的内存,ax和cx中是以KB为单位的内存数量,将其转换为以byte为单位
   mov cx,0x400	     ;cx和ax值一样,cx用做乘数
   mul cx 
   shl edx,16
   and eax,0x0000FFFF
   or edx,eax
   add edx, 0x100000 ;ax只是15MB,故要加1MB
   mov esi,edx	     ;先把低15MB的内存容量存入esi寄存器备份

;2 再将16MB以上的内存转换为byte为单位,寄存器bx和dx中是以64KB为单位的内存数量
   xor eax,eax
   mov ax,bx		
   mov ecx, 0x10000	;0x10000十进制为64KB
   mul ecx		    ;32位乘法,默认的被乘数是eax,积为64位,高32位存入edx,低32位存入eax.
   add esi,eax		;由于此方法只能测出4G以内的内存,故32位eax足够了,edx肯定为0,只加eax便可
   mov edx,esi		;edx为总内存大小
   jmp .mem_get_ok

;-----------------  int 15h ah = 0x88 获取内存大小,只能获取64M之内  ----------
.e801_failed_so_try88: 
   ;int 15后，ax存入的是以kb为单位的内存容量
   mov  ah, 0x88
   int  0x15
   jc .error_hlt
   and eax,0x0000FFFF
      
   ;16位乘法，被乘数是ax,积为32位.积的高16位在dx中，积的低16位在ax中
   mov cx, 0x400     ;0x400等于1024,将ax中的内存容量换为以byte为单位
   mul cx
   shl edx, 16	     ;把dx移到高16位
   or edx, eax	     ;把积的低16位组合到edx,为32位的积
   add edx,0x100000  ;0x88子功能只会返回1MB以上的内存,故实际内存大小要加上1MB

.mem_get_ok:
   mov [total_mem_bytes], edx	 ;将内存换为byte单位后存入total_mem_bytes处。


;-----------------   准备进入保护模式   -------------------
;1 打开A20
;2 加载gdt
;3 将cr0的pe位置1

   ;-----------------  打开A20  ----------------
   in al,0x92
   or al,0000_0010B
   out 0x92,al

   ;-----------------  加载GDT  ----------------
   lgdt [gdt_ptr]

   ;-----------------  cr0第0位置1  ----------------
   mov eax, cr0
   or eax, 0x00000001
   mov cr0, eax

   jmp dword SELECTOR_CODE:p_mode_start	   ; 刷新流水线，避免分支预测的影响,这种cpu优化策略，最怕jmp跳转，
					                     ; 这将导致之前做的预测失效，从而起到了刷新的作用。
.error_hlt:		      ;出错则挂起
   hlt

[bits 32]
p_mode_start:
   mov ax, SELECTOR_DATA
   mov ds, ax
   mov es, ax
   mov ss, ax
   mov esp,LOADER_STACK_TOP
   mov ax, SELECTOR_VIDEO
   mov gs, ax

   ; 创建页目录及页表并初始化内存位图
   call setup_page
  
   ; 要将描述符表地址及偏移量写入内存gdt_ptr,一会儿用新地址重新加载
   sgdt [gdt_ptr] ; 储存到原来gdt所有位置
   
   ; 将gdt描述符中视频段描述符中的段基址+0xc0000000
   mov ebx, [gdt_ptr + 2] ; gdt地址
   or dword [ebx + 0x18 + 4], 0xc0000000
   ; 视频段是第3个段描述符,每个描述符是8字节,故0x18
   ; 段描述符的高4字节的最高位是段基址的第31~24位
   
   ; 将gdt的基址加上0xc0000000使其成为内核所在的高地址
   add dword [gdt_ptr + 2], 0xc0000000
   add esp, 0xc0000000 ; 将栈指针同样映射到内核地址

   ; 把页目录地址赋给cr3
   mov eax, PAGE_DIR_TABLE_POS
   mov cr3, eax
   
   ; 打开cr0的pg位(第31位)
   mov eax, cr0
   or eax, 0x80000000
   mov cr0, eax
   
   ; 在开启分页后，用gdt新的地址重新加载
   lgdt [gdt_ptr] ; 重新加载
   
   mov byte [gs:160], 'V'
   ; 视频段段基址已经被更新,用字符V表示virtual addr
   jmp $
   
;-------------   创建页目录及页表   ---------------
; 创建页目录及页表
setup_page:
; 先把页目录占用的空间逐字节清零
	mov ecx, 4096        ;4KB*1024  每一项4字节，总共1024项
	mov esi, 0
.clear_page_dir:
	mov byte [PAGE_DIR_TABLE_POS + esi], 0   ;PAGE_DIR_TABLE_POS为0x100000，这是出了低端1MB的                                               第一个字节,这也是页目录表的位置
	inc esi
	loop .clear_page_dir

; 开始创建页目录项(PDE)
.create_pde:        ; 创建PDE
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x1000 ; 此时eax为第一个页表的位置及属性，因为页目录表大小为4B*1024=4KB=0x1000
	mov ebx, eax    ; 此处为ebx赋值,是为.create_pte做准备,ebx为基址

; 下面将页目录项0和0xc00都存为第一个页表的地址，每个页表表示4MB内存
; 这样0xc03fffff以下的地址和0x003fffff以下的地址都指向相同的页表
; 这是为将地址映射为内核地址做准备
	or eax, PG_US_U | PG_RW_W | PG_P      ; 页目录项的属性RW和P位为1,US为1,表示用户属性,所有特										权级别都可以访问.
	mov [PAGE_DIR_TABLE_POS + 0x0], eax   ; 第1个目录项,在页目录表中的第1个目录项写入第一个页表的位置(0x101000)及属性(7)
	mov [PAGE_DIR_TABLE_POS + 0xc00], eax ; 一个页表项占用四字节
	; 0xc00表示第768个页表占用的目录项,0xc00以上的目录项用于内核空间
	; 也就是页表的0xc0000000~0xffffffff这1G属于内核
	; 0x0~0xbfffffff这3G属于用户进程
	sub eax, 0x1000
	mov [PAGE_DIR_TABLE_POS + 4092], eax  ; 使最后一个目录项指向页目录表自己的地址
	
; 下面创建页表项(PTE)
	mov ecx, 256	; 1M低端内存 / 每页大小 4K = 256
	mov esi, 0
	mov edx, PG_US_U | PG_RW_W | PG_P	; 属性为7
.create_pte:		; 创建PTE
	mov [ebx+esi*4], edx ; 此时的ebx为0x101000,也就是第一个页表的地址
	add edx, 4096
	inc esi
	loop .create_pte
	
; 创建内核其他页表的PDE
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x2000						; 此时eax为第二个页表的位置0x102000
	or eax, PG_US_U | PG_RW_W | PG_P	; 属性为7
	mov ebx, PAGE_DIR_TABLE_POS
	mov ecx, 254						; 范围为第769~1022的所有目录项数量,此处为内核使用部分
	mov esi, 769
.create_kernel_pde:
	mov [ebx+esi*4], eax                 ;mov 0x100000+769*4，102007
	inc esi
	add eax, 0x1000						;此处更新页表的物理地址，讲其写入页目录下中
	loop .create_kernel_pde 
	ret
```

编译运行，其中编译count的参数根据实际大小调整，这里我编译设置的是3，运行结果如下图，其中红框中gdt段基址已经修改为大于`0xc0000000`，也就是3GB之上的内核地址空间，通过`info tab`可查看地址映射关系，其中箭头左边是虚拟地址，右边是对应的物理地址。

![image-20200819003656292](https://inews.gtimg.com/newsapp_ls/0/13412154076/0)

### 总结虚拟地址获取物理地址的过程：

先从**CR3寄存器**中获取页目录表的物理地址，然后用虚拟地址的高 10 位乘以 4 的积作为在页目录中的偏移量去寻址目录项 PDE，从 PDE 中读出页表的物理地址，然后再用虚拟地址的中间 10 位乘以 4 作为该页表中的偏移量去寻址页表项 PTE，从 PTE 中读出页框的物理地址，用虚拟地址的低 12 位作为该物理页框的偏移量。

### 快表TLB

因为从虚拟地址映射到物理地址确实比较麻烦，所以为了提高效率，intel自然想得到用一个缓存装置TLB。结构如下，更新TLB的方法有两种，重新加载CR3和指令`invlpg m`，其中m表示操作数为虚拟内存地址，如更新虚拟地址0x1234对应的条目指令为`invlpg [0x1234]`

![image-20200819004403471](https://inews.gtimg.com/newsapp_ls/0/13412154434/0)

### ELF格式浅析

我们下一步的目标是在内核中使用C语言，因为C语言是高级语言，在内核中的C语言用gcc编译需要指定很多参数，避免编译器添加许多不必要的函数。然而在Linux下C语言编译而成的可执行文件格式为ELF，想在我们的内核中运行ELF程序首先需要对其进行解析，下面简单介绍一下ELF文件格式，ELF文件格式分为文件头和文件体部分，文件头存放程序中其他的一些头表信息，文件体则具体的对这些表进行描述。ELF格式的作用体现在链接阶段和运行阶段两个方面，其布局如下图所示

![image-20200819004919753](https://inews.gtimg.com/newsapp_ls/0/13412154760/0)

其中elf header的结构如下所示，这里的很多结构都来自Linux源码`/usr/include/elf.h`

```c
/* 32位elf头 */
struct Elf32_Ehdr
{
    unsigned char e_ident[16];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
};
```

其中的一些数据类型如下

![image-20200819005052892](https://inews.gtimg.com/newsapp_ls/0/13412155162/0)

下面介绍一些关键成员，其中`e_ident[16]`数组功能如下，其大小是16字节，存放一些文件属性信息

![image-20200819005142913](https://inews.gtimg.com/newsapp_ls/0/13412155512/0)

`e_type`占用2字节，指定 elf 目标文件的类型

![image-20200819005256943](https://inews.gtimg.com/newsapp_ls/0/13412155840/0)

![image-20200819005306159](https://inews.gtimg.com/newsapp_ls/0/13412156273/0)

剩下的一些字段如下，想更具体了解的可以自己百度

![image-20200819005449948](https://inews.gtimg.com/newsapp_ls/0/13412156599/0)

介绍下**程序头表**中的条目的数据结构，这是用来描述各个段的信息用的，其结构名为 `struct Elf32_Phdr`

![image-20200819005708593](https://inews.gtimg.com/newsapp_ls/0/13412156992/0)

各个字段意义如下

![image-20200819005736164](https://inews.gtimg.com/newsapp_ls/0/13412157365/0)

### 载入内核

Linux下可以用`readelf`命令解析ELF文件，下面是我们在kernel目录下新添加的测试代码，因为是64位操作系统，编译命令需要如下修改，我们下一步就是**将这个简单的elf文件加载入内核**，物理内存中0x900是loader.bin的加载地址，其开始部分是不能覆盖的GDT，预计其大小是小于2000字节，保守起见这里选起始的物理地址为0x1500，所以链接命令指定虚拟起始地址0xc0001500

![image-20200819012138552](https://inews.gtimg.com/newsapp_ls/0/13412157700/0)

下面通过`dd`命令将其写入磁盘，为了不纠结count的赋值，这里直接赋值为200，即写入200个扇区，seek赋值为9，写在第9扇区

```
sudo dd if=./kernel.bin of=/home/qdl/bochs-2.6.2/bin/hd60M.img bs=512 count=200 seek=9 conv=notrunc
```

写完之后我们需要修改loader.S中的内容，分两步完成

- 加载内核：内核文件加载到内存缓冲区
- 初始化内核：需要在分页后，将加载进来的elf内核文件安置到相应的虚拟内存地址，然后跳过去执行，从此loader的工作结束

![image-20200819012715336](https://inews.gtimg.com/newsapp_ls/0/13412158440/0)

由上图知道我们的可用内存，内核的加载地址选取的是`0x7e00~0x9fbff`范围中的`0x70000`，添加如下片断

```
; ------------------ 加载内核 ------------------
mov eax, KERNEL_START_SECTOR  ; kernel.bin所在的扇区号0x9    
mov ebx, KERNEL_BIN_BASE_ADDR ; 0x70000 此处两个宏需要在boot.inc中添加
; 从磁盘读出后,写入到ebx指定的地址
mov ecx, 200 ; 读入的扇区数

call rd_disk_m_32 ; eax,ebx,ecx均为参数,从硬盘上读取数据

; 创建页目录及页表并初始化页内存位图
call setup_page
```

下一步是初始化内核的工作，我们需要遍历`kernel.bin`程序中所有的段，因为它们才是程序运行的实质指令和数据的所在地，然后将各段拷贝到自己被编译的虚拟地址中，如下添加的是在`loader.S`中的内容，注释已经很详细了

```
; -------------------------   加载kernel  ----------------------
   [略...]
   ; 打开cr0的pg位(第31位)
   mov eax, cr0
   or eax, 0x80000000
   mov cr0, eax
   
   ; 在开启分页后，用gdt新的地址重新加载
   lgdt [gdt_ptr] ; 重新加载
   
   jmp SELECTOR_CODE:enter_kernel	  ; 强制刷新流水线,更新gdt,不刷新也可以
enter_kernel:  
   call kernel_init
   mov esp, 0xc009f000     ;进入内核之后栈也要修改
   jmp KERNEL_ENTRY_POINT  ; 用地址0x1500访问测试，结果ok
;----------将kernel.bin中的segment拷贝到编译的地址----------
kernel_init:
   xor eax, eax
   xor ebx, ebx	; 记录程序头表地址
   xor ecx, ecx	; cx记录程序头表中的program header数量
   xor edx, edx	; dx记录program header尺寸,即e_phentsize
	
   mov dx, [KERNEL_BIN_BASE_ADDR + 42] ; 偏移文件42字节处的属性是e_phentsize,表示program header大小
   mov ebx, [KERNEL_BIN_BASE_ADDR + 28] ; 偏移文件开始部分28字节的地方是e_phoff,表示第1个program header在文件中的偏移量

   add ebx, KERNEL_BIN_BASE_ADDR
   mov cx, [KERNEL_BIN_BASE_ADDR + 44]    ; 偏移文件开始部分44字节的地方是e_phnum,表示有几个program header
.each_segment:
   cmp byte [ebx + 0], PT_NULL ; 若p_type等于 PT_NULL,说明此program header未使用。
   je .PTNULL
   
   ;为函数memcpy压入参数,参数是从右往左依然压入.函数原型类似于 memcpy(dst,src,size)
   push dword [ebx + 16] ; program header中偏移16字节的地方是p_filesz,压入函数memcpy的第三个参数:size
   mov eax, [ebx + 4] ; 距程序头偏移量为4字节的位置是p_offset
   add eax, KERNEL_BIN_BASE_ADDR	  ; 加上kernel.bin被加载到的物理地址,eax为该段的物理地址
   push eax
   push dword [ebx + 8] ; 压入函数memcpy的第一个参数:目的地址,偏移程序头8字节的位置是p_vaddr，这就是目的地址
   call mem_cpy ; 调用mem_cpy完成段复制
   add esp,12   ; 清理栈中压入的三个参数, 3 * 4 = 12 字节
.PTNULL:
   add ebx, edx				  ; edx为program header大小,即e_phentsize,在此ebx指向下一个program header 
   loop .each_segment
   ret
   
;----------  逐字节拷贝 mem_cpy(dst,src,size) ------------
;输入:栈中三个参数(dst,src,size)
;输出:无
;---------------------------------------------------------
mem_cpy:
	cld ; 控制重复字符递增方式,也就是edi和esi每复制一次就加一个单位大小,相对的指令为std
	push ebp
	mov esp, ebp
	push ecx ; rep指令用到了ecx，但ecx对于外层段的循环还有用，故先入栈备份
	mov edi, [ebp + 8]  ; dst
	mov esi, [ebp + 12] ; src
	mov ecx, [ebp + 16] ; size
	rep movsb ; 逐字节拷贝,直到ecx为0
	
	; 恢复环境
	pop ecx
	pop ebp
	ret
```

最终的一个内存布局如下，参考之前的1MB实模式地址图来对应就明白了

![image-20200819131311809](https://inews.gtimg.com/newsapp_ls/0/13412159074/0)

### 特权管理

特权级按照权力分为0、1、2、3级，数字越小，级别越高。计算机启动之初就在0级特权运行，MBR则就是0级权限，谈到权限就得提到TSS任务状态段，程序拥有此结构才能运行，相当于一个任务的身份证，结构如下图所示，大小为104字节，其中有很多寄存器信息，而TSS则是由TR寄存器加载的。

![image-20200819132048072](https://inews.gtimg.com/newsapp_ls/0/13412159389/0)

![image-20200819132115676](https://inews.gtimg.com/newsapp_ls/0/13412159716/0)

每个特权级只能有一个栈，特权级在变换的时候需要用到不同特权级下的栈，特权转移分为两类，**一类是中断门和调用门实现低权限到高权限**，**另一类是由调用返回指令从高权限到低权限**，这是**唯一**一种让处理器降低权限的方法。

- 对于低权限到高权限的情况，处理器需要提前记录目标栈的地方，更新SS和ESP，也就是说我们只需要提前在TSS中记录好高特权级的栈地址即可，也就是说TSS不需要记录3级特权的栈，因为它的权限最低。
- 对于高权限到低权限的情况，一方面因为处理器不需要在TSS中寻找低特权级目标栈的，也就是说TSS也不需要记录3级特权的栈，另一方面因为低权限的栈地址已经存在了，这是由处理器的向高特权级转移指令(int、call等)实现机制决定的。

下面就介绍一下权限相关的一些知识点：

**CPL、DPL、RPL**

- CPL是当前进程的权限级别(Current Privilege Level)，是当前正在执行的代码所在的段的特权级，存在于cs寄存器的低两位，也就是第 0 位和第 1 位上，通常情况下，CPL等于代码所在段的特权等级，当程序转移到不同的代码段时，处理器将改变CPL。
- RPL是进程对段访问的请求权限(Request Privilege Level)，是对于段选择子而言的，每个段选择子有自己的RPL，它是通过段选择子的低两位表现出来的，CS和SS也是存放选择子的，同时CPL存放在CS和SS的低两位上，那么对于CS和SS来说，选择子的RPL=当前段的CPL。
- DPL存储在段描述符中，规定访问该段的权限级别(Descriptor Privilege Level)，每个段的DPL固定。当进程访问一个段时，需要进程特权级检查，一般要求DPL >= max {CPL, RPL}

### 门结构

处理器只有通过门结构才能由低特权级转移到高特权级，也可以通过门结构进行平级跳转，所以门相当于一个跳板，当前特权级首**先需要大于门的DPL特权级**，然后才能使用门来跳到想去的特权级，处理器就是这样设计的，四种门结构分别是：任务门、中断门、陷阱门、调用门。门描述符和段描述符类似，都是8字节大小的数据结构，用来描述门通向的代码，如下所示

![image-20200819140954194](https://inews.gtimg.com/newsapp_ls/0/13412160197/0)

![image-20200819141011231](https://inews.gtimg.com/newsapp_ls/0/13412160767/0)

任务门可以放在GDT、LDT、IDT中，调用门位于GDT、LDT中，中断门和陷阱门仅位于IDT中调用方法如下

**调用门**

call 和 jmp 指令后接调用门选择子为参数，以调用函数例程的形式实现从低特权向高特权转移，可用来实现系统调用。 call 指令使用调用门可以实现向高特权代码转移， jmp 指令使用调用门只能实现向平级代码转移。若需要参数传递，则0~4位表示参数个数，然后在权限切换的时候自动在栈中复制参数。关于调用门的过程保护，参考P240

**中断门**

以 int 指令主动发中断的形式实现从低特权向高特权转移， Linux 系统调用便用此中断门实现。

**陷阱门**

以 int3 指令主动发中断的形式实现从低特权向高特权转移，这一般是编译器在调试时用。

**任务门**

任务以任务状态段 TSS 为单位，用来实现任务切换，它可以借助中断或指令发起。当中断发生时，如果对应的中断向量号是任务门，则会发起任务切换。也可以像调用门那样，用 call 或 jmp 指令后接任务门的选择子或任务 TSS 的选择子。

### IO特权级

保护模式下，处理器中的”阶级”不仅体现在数据和代码的访问，还体现在以下只有在0特权级下被执行的特权指令

```
hlt、lgdt、ltr、popf等
```

还有一些IO敏感指令如`in、out、cli、sti`等访问端口的指令也需要在相应的特权级下操作，如果当前特权级小于 IOPL 时就会产生异常，IOTL 在 eflags 寄存器中，没有特殊的指令设置 eflags 寄存器，只能用 popf 结合 iretd 指令，在栈中将其修改，当然也只有在0特权下才能操作，eflags 寄存器中的 IOTL 位如下所示

![image-20200819142009071](https://inews.gtimg.com/newsapp_ls/0/13412161285/0)