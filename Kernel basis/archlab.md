## part A

这部分的任务就是用`Y86-64`指令集完成`example.c`中`sum_list`、`rsum_list`和`copy_block`这三个函数的编写

### 数据结构

```c
/* linked list element */
typedef struct ELE {
    long val;
    struct ELE *next;
} *list_ptr;
```

```text
  .align 8
Elle
  .quad 0x00a
  .quad ele2
ele2:
  .quad 0x0b0
  .quad ele3
ele3:
  .quad 0xc00
  .quad 0 
```

### sum_list

```
#函数开始
    .pos   0
init:
    irmovq stack,%rsp #设置栈指针
    call main
    halt
#链表
    .align 8
ele1:
    .quad 0x00a
    .quad ele2
ele2:
    .quad 0x0b0
    .quad ele3
ele3:
    .quad 0xc00
    .quad 0

main:
    irmovq ele1,%rdi
    call sum_list
    ret

sum_list:
    pushq %rbx             #保存使用前的数据
    xorq %rax,%rax         #val = 0;
    jmp test

loop:
    mrmovq (%rdi),%rbx     #保存ls->val
    addq %rbx,%rax         #val += ls->val
    mrmovq 8(%rdi),%rdi    #ls = ls->next

test:
    andq %rdi,%rdi         #判断ls是否为零
    jne loop               #ls非零
    popq %rbx              #恢复保存前的数据
    ret

#设置栈地址
    .pos 0x100
stack:
```

运行结果

```
linux> ./yas sum.ys
linux> ./yis sum.yo
Stopped in 28 steps at PC = 0x13.  Status 'HLT', CC Z=1 S=0 O=0
Changes to registers:
%rax:	0x0000000000000000	0x0000000000000cba
%rsp:	0x0000000000000000	0x0000000000000100

Changes to memory:
0x00f0:	0x0000000000000000	0x000000000000005b
0x00f8:	0x0000000000000000	0x0000000000000013
```

### rsum_list

```c
long rsum_list(list_ptr ls)
{
    if (!ls)
	return 0;
    else {
	long val = ls->val;
	long rest = rsum_list(ls->next);
	return val + rest;
    }
} 
```

与sum_list相似

```
#函数开始
    .pos   0
init:
    irmovq stack,%rsp #设置栈指针
    call main
    halt
#链表
    .align 8
ele1:
    .quad 0x00a
    .quad ele2
ele2:
    .quad 0x0b0
    .quad ele3
ele3:
    .quad 0xc00
    .quad 0

main:
    irmovq ele1,%rdi
    call rsum_list
    ret

rsum_list:
    pushq %rbx             #保存使用前的数据
    xorq %rax,%rax         #val = 0;
    andq %rdi,%rdi         #判断ls是否为零
    je end                 #ls非零
    mrmovq (%rdi),%rbx     #val = ls->val
    mrmovq 8(%rdi),%rdi    #rest = rsum_list(ls->next);
    call rsum_list
    addq %rbx,%rax         #val + rest
end:
    popq %rbx              #恢复保存前的数据
    ret

#设置栈地址
    .pos 0x100
stack:
```

运行结果

```
linux> ./yas rsum.ys
linux> ./yis rsum.yo
Stopped in 42 steps at PC = 0x13.  Status 'HLT', CC Z=0 S=0 O=0
Changes to registers:
%rax:	0x0000000000000000	0x0000000000000cba
%rsp:	0x0000000000000000	0x0000000000000100

Changes to memory:
0x00b8:	0x0000000000000000	0x0000000000000c00
0x00c0:	0x0000000000000000	0x0000000000000088
0x00c8:	0x0000000000000000	0x00000000000000b0
0x00d0:	0x0000000000000000	0x0000000000000088
0x00d8:	0x0000000000000000	0x000000000000000a
0x00e0:	0x0000000000000000	0x0000000000000088
0x00f0:	0x0000000000000000	0x000000000000005b
0x00f8:	0x0000000000000000	0x0000000000000013

```

### copy_block

```c
long copy_block(long *src, long *dest, long len)
{
    long result = 0;
    while (len > 0) {
	long val = *src++;
	*dest++ = val;
	result ^= val;
	len--;
    }
    return result;
} 
```

将下列块作为输入

```text
  .align 8
# Source block
src:
  .quad 0x00a
  .quad 0x0b0
  .quad 0xc00
# Destination block
dest:
  .quad 0x111
  .quad 0x222
  .quad 0x333
```

```
#函数开始
    .pos   0
init:
    irmovq stack,%rsp #设置栈指针
    call main
    halt

#输入数据
  .align 8
# Source block
src:
  .quad 0x00a
  .quad 0x0b0
  .quad 0xc00
# Destination block
dest:
  .quad 0x111
  .quad 0x222
  .quad 0x333

main:
    irmovq src,%rdi
    irmovq dest,%rsi
    irmovq $3,%rdx
    call copy_block
    ret

copy_block:                  #long copy_block(long *src, long *dest, long len)
    pushq %rbx               #用来保存val
    pushq %r12
    pushq %r13
    xorq %rax,%rax           #result = 0
    irmovq $8,%r12
    irmovq $1,%r13
loop:
    andq %rdx,%rdx           #判断len是否大于0
    jle end
    mrmovq (%rdi),%rbx       #val = *src
    rmmovq %rbx,%rdx         #dest = val
    xorq %rbx,%rax           #result ^= val
    addq %r12,%rdi           #src++
    addq %r12,%rsi           #dest++
    subq %r13,%rdx           #len--
    jmp loop

end:
    popq %r13
    popq %r12
    popq %rbx
    ret

#设置栈地址
    .pos 0x100
stack:
```

这里值得注意的是Y86-64指令集不包含立即数与寄存器之间的运算指令，所以要先将立即数保存进寄存器中，再用寄存器来计算。

```
linux> ./yas copy.ys
linux> ./yis copy.yo
Stopped in 47 steps at PC = 0x13.  Status 'HLT', CC Z=1 S=0 O=0
Changes to registers:
%rax:	0x0000000000000000	0x0000000000000cba
%rsp:	0x0000000000000000	0x0000000000000100
%rsi:	0x0000000000000000	0x0000000000000048
%rdi:	0x0000000000000000	0x0000000000000030

Changes to memory:
0x0000:	0x000000000100f430	0x0000000000000c00
0x00f0:	0x0000000000000000	0x000000000000006f
0x00f8:	0x0000000000000000	0x0000000000000013

```

## part B

该部分在`sim/seq`文件夹中，我们需要对SEQ处理器进行扩展，使其支持`iaddq`指令，该指令可实现常数值V与寄存器rB相加

![QQ图片20200725010638.png](https://i.loli.net/2020/07/26/Yxr6zDXl2E3ycBp.png)

![image-20200725012331798.png](https://i.loli.net/2020/07/26/aZwEktIneSb6Ohj.png)

我们需要在`seq-full.hcl`文件中进行修改，使其能够合法运行`iaddq`指令

```
#取指阶段
##将IIADDQ加入进去 使其变成合法指令
bool instr_valid = icode in 
   { INOP, IHALT, IRRMOVQ, IIRMOVQ, IRMMOVQ, IMRMOVQ,
      IOPQ, IJXX, ICALL, IRET, IPUSHQ, IPOPQ, IIADDQ };
##IIADDQ需要寄存器所以加入进去
bool need_regids =
	icode in { IRRMOVQ, IOPQ, IPUSHQ, IPOPQ, 
		     IIRMOVQ, IRMMOVQ, IMRMOVQ, IIADDQ };
##IIADDQ需要立即数所以加入进去
bool need_valC =
	icode in { IIRMOVQ, IRMMOVQ, IMRMOVQ, IJXX, ICALL, IIADDQ };

#译码阶段和写回阶段
##IIADDQ需要rB寄存器
word srcB = [
	icode in { IOPQ, IRMMOVQ, IMRMOVQ, IIADDQ } : rB;
	icode in { IPUSHQ, IPOPQ, ICALL, IRET } : RRSP;
	1 : RNONE;  # Don't need register
];
##参考过程可知计算后的结果valE保存到寄存器rB中
word dstE = [
	icode in { IRRMOVQ } && Cnd : rB;
	icode in { IIRMOVQ, IOPQ, IIADDQ } : rB;
	icode in { IPUSHQ, IPOPQ, ICALL, IRET } : RRSP;
	1 : RNONE;  # Don't write any register
];

#执行阶段
##iaddq指令需要将aluA的值设置为valC
word aluA = [
	icode in { IRRMOVQ, IOPQ } : valA;
	icode in { IIRMOVQ, IRMMOVQ, IMRMOVQ, IIADDQ } : valC;
	icode in { ICALL, IPUSHQ } : -8;
	icode in { IRET, IPOPQ } : 8;
	# Other instructions don't need ALU
];
##iaddq指令需要将aluB的值设置为valB
word aluB = [
	icode in { IRMMOVQ, IMRMOVQ, IOPQ, ICALL, 
		      IPUSHQ, IRET, IPOPQ, IIADDQ } : valB;
	icode in { IRRMOVQ, IIRMOVQ } : 0;
	# Other instructions don't need ALU
];
##更新CC
bool set_cc = icode in { IOPQ, IIADDQ };
```

- 根据`seq-full.hcl`文件构建新的仿真器

```text
linux> make VERSION=full
```

> 如果不含有`Tcl/Tk`，需要在`Makefile`中将对应行注释掉

- 在小的Y86-64程序中测试你的方法

```text
linux> ./ssim -t ../y86-code/asumi.yo
```

- 测试除了`iaddq`以外的所有指令

```text
linux> (cd ../ptest; make SIM=../seq/ssim)
```

- 测试实现的`iaddq`指令

```text
linux> (cd ../ptest; make SIM=../seq/ssim TFLAGS=-i)

./optest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 58 ISA Checks Succeed
./jtest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 96 ISA Checks Succeed
./ctest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 22 ISA Checks Succeed
./htest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 756 ISA Checks Succeed
```

## **part C**

**//TODO**