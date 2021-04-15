## 保护模式初探

### 实模式到保护模式

#### 寄存器的扩展

![image-20200814104832012.png](https://i.loli.net/2020/10/04/5EcXeSGaUfxgOrh.png)

#### 寻址方式改变

![image-20200814105014090.png](https://i.loli.net/2020/10/04/ODaQAbkjv5yKw9f.png)

32位CPU既支持实模式有支持保护模式，为了区分当前指令到底是哪个模式下运行的，编译器提供了伪指令`bits`

> 指令格式：[bits 16]或[bits 32]，分别对应16位和32位

```
[bits 16]
mov ax, 0x1234
mov dx, 0x1234

[bits 32]
mov eax, 0x1234
mov edx, 0x1234
```

#### 反转前缀

1. 0x66反转前缀(操作数模式的转变)
   假如当前运行模式是16位实模式，操作数大小变为32位。
   假设当前运行模式是32位保护模式，操作数大小变为16位。

   ![image-20200814105357229.png](https://i.loli.net/2020/10/04/u1wf9IYChJ8FmeH.png)

2. 0x67反转前缀(寻址方式模式的转变)

   ![image-20200814105523773.png](https://i.loli.net/2020/10/04/Lz4HAoUBXDQ1GSn.png)

> 为什么实例中用了 eax 和 bx 两种寄存器，而不是 ebx 和 bx？因为实模式下的基址只有用寄存器 bx、bp

### 段描述符

保护模式中的段基址不再是像实模式那样直接存放物理地址，段寄存器中要记录32位地址的数据段基址，16位肯定是装不下的，所以段基址都存储在一个数据结构中——全局描述符表。其中每个表项称为段描述符，其大小为64字节，用来描述各个内存段的起始地址、大小、权限等信息。而**保护模式中段寄存器存放的是段选择子 selector** 。如果把全局描述符表当作数组来看的话，段选择子就是数组的下标，用来索引段描述符。该全局描述符表很大，所以放在内存中，由GDTR寄存器指向它。保护模式首先是必须向前兼容的，故其访问内存依然是`段基址:段内偏移`的方式，结合前面总结过实模式的一些安全问题，想要解决这些问题就得既保证向前兼容，又保证安全性。CPU工程师想到的方法就是增加更多的安全属性位，下图即是段描述符格式：

![image-20200813143233835.png](https://i.loli.net/2020/10/04/HMjiRevgBLEkYJN.png)

对于每个字段的用法等，在使用的时候去查阅即可，段描述符无非就是保存一些段的属性(可读、可写、是否存在等)，权限(Ring0-Ring3)，基址，界限范围等信息。其访问内存的形式如下图所示

![image-20200814110010165.png](https://i.loli.net/2020/10/04/vGnUikzs6A2g7mc.png)

### 全局描述符表GDT

全局描述符表GDT相当于是一个描述符的数组，数组每一个元素都是8个字节的描述符，而选择子则是提供下标在GDT中索引描述符。假设 A[10] 数组即为GDT表，则

- GDT表相当于数组A
- 数组中每个数据A[0]~A[10]相当于描述符
- A[0]~A[10]中的0~10索引下标则是选择子

全局描述符表是公用的，GDTR这个专门的寄存器则存放GDT表的内存地址和大小，是一个48位的寄存器，对这个寄存器操作无法用mov等指令，这里用的是`lgdt`指令初始化，指令格式是：`lgdt 48位内存数据`

![image-20200814110941499.png](https://i.loli.net/2020/10/04/AqYIX5JNBEZfLWi.png)

其中前16位是GDT以字节为单位的界限值，相当于GDT字节大小减1。后32位是GDT的起始地址。由于GDT的大小是16位二进制，表示范围是2^16 = 65536字节。每个描述符大小是8字节，故GDT中最多可容纳的描述符数量是`65536/8 = 8198`，也就是可以容纳8192个段或门。

### 局部描述符表LDT

按照CPU的设想，一个任务对应一个局部描述符表LDT，切换任务的时候也会切换LDT，LDT也存放在内存中，由LDTR寄存器指向，加载的指令为`lldt`。对于操作系统来说，每个系统必须定义一个GDT，用于系统中的所有任务和程序。可选择性定义若干个LDT。LDT本身是一个段，而GDT不是。这种表在这里并不涉及所以不在述说。

### 段选择子

段描述符有了，描述符表也有了，我们该如何使用它呢？下面就要用到段的选择子，其结构如下图所示。

![image-20200814111920658.png](https://i.loli.net/2020/10/04/Hqlzgr1KDeSsBb7.png)

- 其低2位即0~1位，用来存储RPL，即请求特权级，可以表示0、1、2、3四种特权级。

- 第2位是TI位，即Table Indicator，TI为0表示在GDT中索引描述符，TI为1表示在LDT中索引描述符。

- 选择子的高13位，即第3~15位是描述符的索引值，前面说过 GDT 相当于一个描述符数组，所以此选择子中 的索引值就是 GDT 中的下标。

  > 由于选择子的索引值部分是 13 位，即 2 的 13 次方是 8192，故多可以索引 8192 个段，这和 GDT 中多定义 8192 个描述符是吻合的

例如选择子是 0x8，将其加载到 ds 寄存器后，访问 ds：0x9 这样的内存，其过程是：0x8 的低 2 位是 RPL，其值为 00。第 2 是 TI，其值 0，表示是在 GDT 中索引段描述符。用 0x8 的高 13 位 0x1 在 GDT 中 索引，也就是 GDT 中的第 1 个段描述符（GDT 中第 0 个段描述符不可用）。假设第 1 个段描述符中的 3 个段基址部分，其值为0x1234。CPU将0x1234作为段基址，与段内偏移地址0x9相加，0x1234+0x9=0x123d。 用所得的和 0x123d 作为访存地址。

> GDT中第0个段描述符不可用是为了防止未初始化段选择子，如果未初始化段选择子就会访问到第0个段描述符从而抛出异常。

为了让`段基址:段内偏移`策略继续可用，CPU采取的做法是将超过1MB的部分自动绕回到0地址，继续从0地址开始映射。相当于把地址对1MB求模。超过1MB多余出来的内存被称为高端内存区HMA。

这种地址绕回的做法需要通过两种情况分别讨论：

- 对于只有20位地址线的CPU，不需要任何操作便能自动实现地址绕回
- 当其他有更多地址总线的时候，因为CPU可以访问更多的内存，所以不会产生地址回滚。这种情况下的解决方案就是对第21根地址线进行操作。开启A20则直接访问物理地址即可，关闭A20则使用回绕方式访问。

打开A20的操作方法有以下三个步骤，主要是将0x92端口第一位置一即可

```
in al, 0x92
or al, 0000_0010B
out 0x92, al
```

### CR0寄存器

控制寄存器是 CPU 的窗口，既可以用来展示 CPU 的内部状态，也可用于控制 CPU 的运行机制。这次我们要用到的是 CR0 寄存器。更准确地说，我们要用到 CR0 寄存器的第 0位，即 PE 位，Protection Enable，此位用于启用保护模式，是保护模式的开关。当打开此位后，CPU 才 真正进入保护模式，所以这是进入保护模式三步中的 后一步。

![image-20200814135918877.png](https://i.loli.net/2020/10/04/4I9t7YGnMUxyvEk.png)

关于其他字段信息如下

![image-20200814135944324.png](https://i.loli.net/2020/10/04/tQv9DBjhNkirluZ.png)

PE 为 0 表示在实模式下运行，PE 为 1 表示在保护模式下运行。对CR0的PE位置1如下所示。

```
mov eax,cr0
or eax,0x00000001
mov cr0,eax
```

### 进入保护模式

接下来是实验阶段，我们需要更新我们的mbr和loader文件，因为我们的`loader.bin`会超过512字节，所以要增大扇区，目前是1扇区，我们改为4扇区。

```
...
52 mov cx, 4         ; 带读入的扇区数
53 call rd_disk_m_16 ; 以下读取程序的起始部分(一个扇区)
...
```

cx 寄存器中存放的这个参数非常重要，代表读入扇区数，如果`loader.bin`的大小超过mbr读入的扇区数，就需要对这个参数进行修改

接下来就是更新`boot.inc`，里面存放的是`loader.S`的一些符号信息，相当于头文件，比之前主要多定义了GDT描述符的属性和选择子的属性。Linux使用的是平坦模型，整个内存都在一个段里，这里平坦模型在我们定义的描述符中，段基址是0，`段界限 * 粒度 = 4G` 粒度选的是4k，故段界限是 0xFFFFF

```
;--------------------- loader 和 kernel---------------------

LOADER_BASE_ADDR equ 0x900
LOADER_START_SECTOR equ 0x2

;--------------------  gdt 描述符属性  ----------------------
DESC_G_4K         equ 1_00000000000000000000000b        ;描述符的G位为4k粒度，以二进制表示，下划线可去掉
DESC_D_32         equ  1_0000000000000000000000b
DESC_L            equ   0_000000000000000000000b        ;64位代码标记，此处标记为0便可
DESC_AVL          equ    0_00000000000000000000b        ;CPU不用此位，暂置为0
DESC_LIMIT_CODE2  equ     1111_0000000000000000b        ;段界限，需要设置为0xFFFFF
DESC_LIMIT_DATA2  equ     DESC_LIMIT_CODE2
DESC_LIMIT_VIDEO2 equ      0000_000000000000000b
DESC_P			  equ         1_000000000000000b
DESC_DPL_0        equ          00_0000000000000b
DESC_DPL_1		  equ          01_0000000000000b
DESC_DPL_2        equ		   10_0000000000000b
DESC_DPL_3        equ          11_0000000000000b
DESC_S_CODE		  equ            1_000000000000b
DESC_S_DATA       equ            DESC_S_CODE
DESC_S_sys        equ            0_000000000000b
DESC_TYPE_CODE    equ             1000_00000000b		;x=1,c=0,r=0,a=0 代码段是可执行的,非一致性,不可读,已访问位a清0.  
DESC_TYPE_DATA    equ             0010_00000000b		;x=0,e=0,w=1,a=0 数据段是不可执行的,向上扩展的,可写,已访问位a清0.

DESC_CODE_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_CODE2 + DESC_P + DESC_DPL_0 + DESC_S_CODE + DESC_TYPE_CODE + 0x00 ;定义代码段的高四字节，(0x00 << 24)表示"段基址的24~31"字段，该字段位于段描述符高四字节24~31位，平坦模式段基址为0，所以这里用0填充，最后的0x00也是
DESC_DATA_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_DATA2 + DESC_P + DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x00
DESC_VIDEO_HIGH4 equ (0x00 << 24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_VIDEO2 + DESC_P + DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x0b

;--------------   选择子属性  ---------------
RPL0  equ   00b
RPL1  equ   01b
RPL2  equ   10b
RPL3  equ   11b
TI_GDT	 equ   000b
TI_LDT	 equ   100b
```

下面修改 `loader.S`

```
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
           dd DESC_CODE_HIGH4    ;boot.inc中定义的高四字节
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
```

同之前的方法编译，注意这里loader.bin编译后为615个字节，需要2个扇区大小，写入磁盘时要给count赋值为2

```
sudo nasm -I include/ -o 4mbr.bin 4mbr.S
sudo nasm -I include/ -o loader.bin loader.S

sudo dd if=/home/qdl/bochs-2.6.2/bin/mbr/boot/4mbr.bin of=/home/qdl/bochs-2.6.2/bin/hd60M.img bs=512 count=1  conv=notrunc

sudo dd if=./loader.bin of=/home/qdl/bochs-2.6.2/bin/hd60M.img bs=512 count=2 seek=2 conv=notrunc
```

运行`sudo ./bochs -f bochsrc.disk`效果如下，其中`1 MBR`来自实模式下的mbr.S，`2 loader in real`是在实模式下用 BIOS 中断 0x10 打印的。左上角第 2 行的 字符`P`，是在保护模式下输出的。一个程序历经两种模式，各模式下都打印了字符，为了区别实模式下的打印，所以字符串中含有`in real`。 

![image-20200814163025116.png](https://i.loli.net/2020/10/04/xNK6wspU3yFJTZl.png)

查看GDT表中的内容和我们设置的相符，其中第0个不可用。查看寄存器信息PE位设置为1表示已经进入保护模式。

![image-20200814164047421.png](https://i.loli.net/2020/10/04/CxL23sA1m59toOH.png)

#### 加载选择子的保护

保护模式中的保护二字体现在哪里？其实主要体现在段描述符的属性字段中。每个字段都不是多余 的。这些属性只是用来描述一块内存的性质，是用来给 CPU 做参考的，当有实际动作在这片内存上发生 时，CPU 用这些属性来检查动作的合法性，从而起到了保护的作用。 

当引用一个内存段时，实际上就是往段寄存器中加载个段选择子，为了避免非法引用内存段的情况，会检查选择子是否合理，判断方法就是通过验证索引值是否出现越界，越界则抛出异常。有如下表达式

> 描述符表基地址+选择子中的索引值*8+7<=描述符表基地址+描述符表界限值

![image-20200814164334865.png](https://i.loli.net/2020/10/04/rmXzpdF7TUuYc1D.png)

检查完选择子就该检查段描述符中 type 字段，也就是段的类型，大致原则如下：

- 只有具备可执行属性的段(代码段)才能加载到CS段寄存器中。
- 只具备执行属性的段（代码段）不允许加载到除 CS 外的段寄存器中。
- 只有具备可写属性的段（数据段）才能加载到 SS 栈段寄存器中。
- 至少具备可读属性的段才能加载到 DS、ES、FS、GS 段寄存器中。 

![image-20200814164612898.png](https://i.loli.net/2020/10/04/74J8qh3mLpxOfz5.png)

检查完段的类型后检查P位，P位表示该段是否存在，1表示存在，0表示不存在。

#### 代码段和数据段的保护

代码段和数据段主要保护措施是当CPU访问一个地址的时候，判断该地址不能超过所在内存段的范围。简单总结如下图所示，出现这种跨段操作就会出现异常。

![image-20200814164833876.png](https://i.loli.net/2020/10/04/4OdwkysmhicYEQt.png)

#### 栈段的保护

段描述符type中的e位表示扩展方向，栈可以向上扩展和向下扩展，下面就是检查方式

- 对于向上拓展的段，实际段界限是段内可以访问的最后一个字节
- 对于向下拓展的段，实际段界限是段内不可以访问的第一个字节

等价于如下表达式

```
实际段界限+1<=esp-操作数大小<=0xFFFFFFFF
```