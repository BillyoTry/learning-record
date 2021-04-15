## 中断

中断的存在极大提高了计算机的效率，可分为外部中断和内部中断。

- 外部中断的中断源为某个硬件，所以外部中断又称为硬件中断。CPU为中断信号提供了两条信号线分别是`INTR`和`NMI`，如下图所示，从INTR引脚收到的中断都是不影响系统运行的，可以随时处理，不会影响到CPU的执行。也称为可屏蔽中断。可以通过eflag中的`IF`位将所有这些外部中断屏蔽。

  ![image-20200822210825242](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200822210825242.png)

- 内部中断可分为软中断和异常

  1. **软中断**

     顾名思义是软件主动发起的中断，不受eflags中的IF位的影响，不能因为 IF 位为 0 就不顾用户请求，所以 为了用户功能正常，软中断必须也无视 IF 位。其有如下指令：

     - int 8位立即数，通过它进行系统调用。
     - int3，int和3之间无空格，用于调试。
     - into，中断溢出指令，当OF位也为1时，触发中断向量号为4的中断。
     - bound，检查数组索引越界指令，越界时触发5号中断。
     - ud2，未定义指令，触发6号中断。

  2. **异常**

     异常是指令执行期间CPU内部产生的错误引起的，也不受eflags中的IF位的影响，按照轻重程度分为三种

     1. Fault，也称故障。属于**可被修复**的一种类型，当发生此类异常时，CPU将机器状态恢复到异常之前的状态 ，之后调用中断处理程序，通常都能够被解决。缺页异常就属于此种异常
     2. Trap，也称陷阱。此异常通常在调试中。
     3. Abort，也称终止。程序发生了此类异常通常就无法继续执行下去，操作系统会将此程序从进程表中去除，某些异常会有单独的错误码，即 error code，进入中断时 CPU 会把它们压在栈中。

  异常与中断见下表

![image-20200822212751605](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200822212751605.png)

### 中断描述表

中断描述符表是保护模式下用于**存储中断处理程序入口的表**，当CPU接受到一个中断时，需要根据该中断的中断向量号在此表中检索对应的描述符，在该描述符中找到中断处理程序的起始地址，然后执行中断处理程序，这和之前段描述符非常类似，类比学习即可。

实模式下用于中断处理程序入口的表叫做中断向量表(IVT)，保护模式下则是中断描述符表(IDT)。

IVT在实模式下位于0~0x3ff共1024个字节，又知IVT可容纳256个中断向量，故每个中断向量用4字节描述；对比IVT，IDT表地址不受限制，在哪里都可以，每个描述符用8字节描述。这里主要讨论IDT，在IDT中描述符称之为门，也就是之前介绍过的门，这里再区别一下门和段描述符。

- 段描述符中描述的是一片内存区域
- 门描述符描述的是一段代码，除调用门外，任务门、中断门、陷阱门都可以存在于中断描述符中

IDT位置不固定，故CPU找到它需要通过一个寄存器**IDTR**，如下图，第 0～15 位是表界限，即 IDT 大小减 1，第 16～47 位是 IDT 的基地址，和之前的GDTR是一个原理。

![image-20200822215019567](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200822215019567.png)

16 位的表界限，表示大范围是 0xffff，即 64KB。可容纳的 描述符个数是 64KB/8=8K=8192 个。特别注意的是 GDT 中的第 0 个段描述符是不可用的，但 IDT 却无此限制，第 0 个门描述符也 是可用的，中断向量号为 0 的中断是除法错。但处理器只支持 256 个中断，即 0～254，中断描述符中其余的描述符不可用。在门描述符中有个 P 位，所以，咱们将来在构 建 IDT 时，记得把 P 位置 0，这样就表示门描述符中的中断处理程序不在内存中。加载IDTR需要用到lidt指令，用法是`lidt 48位内存数据`

这里我们讨论CPU内的中断处理过程总结如下

1. 处理器根据中断向量号定位中断门描述符
2. 处理器进行特权级检查
3. 执行中断处理程序

![image-20200822233656202](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200822233656202.png)

中断发生之后需要执行中断处理程序，该中断处理程序是通过中断门描述符中保存的代码段选择子和段内偏移找到的，这个时候就需要重新加载段寄存器，也就是说需要在栈中保存一些寄存器信息(CS:EIP、eflags等)，保证中断之后执行的流程正确，当**特权级变化**的时候，压栈如下图所示

![image-20200822233816045](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200822233816045.png)

图A、B：在发生中断是通过特权级的检测，发现需要向高特权级转移，所以要保存当前程序栈的SS和ESP的值，在这里记为ss_old, esp_old，然后在新栈中压入当前程序的eflags寄存器。

图C、D：由于要切换目标代码段，这种段间转移，要对CS和EIP进行备份，同样将其存入新栈中。某些异常会有错误码，用来标识异常发生在哪个段上，对于有错误码的情况，要将错误码也压入栈中。

当特权级没有变化的时候，就不需要压入旧栈的SS和ESP，下图就是特权级未发生变化的情况。

![image-20200822234337009](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200822234337009.png)

返回的时候通过指令 **iret** 完成，**iret** 指令会从栈顶依次弹出EIP、CS、EFLAGS，根据特权级的变化还有ESP、SS。但是该指令并不验证数据的正确性，而且他从栈中弹出数据的顺序是不变的，也就是说，在有error_code的情况下，iret返回时并**不会主动跳过这个数据**，需要我们手动进行处理。

### 编写中断处理程序

下面通过操作8259A芯片实现第一个中断处理程序，关于8259A相关信息参考书中内容，本质上是一个可编程中断控制器，处理流程如下，`init_all`负责初始化所有设备及结构体，然后调用`idt_init`初始化中断相关内容，内部分别调用了`pic_init`和`idt_desc_init`实现，其中`pic_init`初始化8259A，`idt_desc_init`负责对中断描述符IDT表进行初始化，最后再对IDT表进行加载

![image-20200822235105783](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200822235105783.png)

我们需要进行以下几个步骤

1. 用汇编语言实现中断处理程序
2. 创建中断描述符表IDT，安装中断处理程序
3. 用内联汇编实现端口I/O函数(对端口的读写操作)
4. 设置8259A

新添加中断后的文件树如下所示，`build`中是生成后的文件，`device`中存放的是为了提高中断频率对8253计数器的操作，`kernel`中新加的`interrupt`是对中断初始化的主要文件

```
.
├── boot
│   ├── include
│   │   └── boot.inc
│   ├── loader.bin
│   ├── loader.S
│   ├── mbr.bin
│   └── mbr.S
├── build
│   ├── init.o
│   ├── interrupt.o
│   ├── kernel.bin
│   ├── kernel.o
│   ├── main.o
│   ├── print.o
│   └── timer.o
├── device
│   ├── timer.c
│   └── timer.h
├── kernel
│   ├── global.h
│   ├── init.c
│   ├── init.h
│   ├── interrupt.c
│   ├── interrupt.h
│   ├── kernel.S
│   └── main.c
└── lib
    ├── kernel
    │   ├── io.h
    │   ├── print.h
    │   ├── print.o
    │   └── print.S
    ├── stdint.h
    └── user
```

编译命令如下

```
//编译c程序，生成目标文件，这里需要关闭栈保护并指定32位程序
sudo gcc -m32 -fno-stack-protector -I lib/kernel/ -c -o build/timer.o device/timer.c
sudo gcc -m32 -fno-stack-protector -I lib/kernel -I lib/ -I kernel -c -fno-builtin -o build/init.o kernel/init.c 
sudo gcc -m32 -fno-stack-protector -I lib/kernel -I lib/ -I kernel -c -fno-builtin -o build/main.o kernel/main.c
sudo gcc -m32 -fno-stack-protector -I lib/kernel -I lib/ -I kernel -c -fno-builtin -o build/interrupt.o kernel/interrupt.c

//编译汇编
sudo nasm -f elf -o build/print.o lib/kernel/print.S
sudo nasm -f elf -o build/kernel.o kernel/kernel.S

//链接，在build目录下
sudo ld -m elf_i386 -Ttext 0xc0001500 -e main -o kernel.bin main.o init.o interrupt.o print.o kernel.o timer.o

//写入img
sudo dd if=./kernel.bin of=/home/guang/soft/bochs-2.6.2/bin/hd60M.img bs=512 count=200 seek=9 conv=notrunc
```

运行结果如下，这里我为了效果演示注释了`interrupt.c`文件中`general_intr_handler`函数的最后三行打印中断号的部分，结果如下

![image-20200823164219253](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200823164219253.png)

取消注释后，效果如下

![image-20200823164620543](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200823164620543.png)