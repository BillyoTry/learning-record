## 内存管理系统

在编写内存管理系统之前需要进行一些其他的准备工作

### Makefile和断言

为了更方便的的对kernel进行编译，这里使用makefile来操作，makefile具体的知识点就不单独列举了，感兴趣的小伙伴可以自己查阅资料，和作者不同的是这里我是x64的系统，新增了一些编译选项并且把ubantu的终端修改为了bash，具体如下

```makefile
BUILD_DIR = ./build   #存储编译生成的所以目标文件
ENTRY_POINT = 0xc0001500
AS = nasm #汇编语言编译器
CC = gcc  #C语言编译器
LD = ld   #链接器
LIB = -I lib/ -I lib/kernel/ -I lib/user/ -I kernel/ -I device/
ASFLAGS = -f elf      #汇编语言编译器参数
CFLAGS = -m32 -fno-stack-protector -Wall $(LIB) -c -fno-builtin -W -Wstrict-prototypes \
         -Wmissing-prototypes       #C语言编译器参数
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map     #链接器参数
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o \
      $(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o \
      $(BUILD_DIR)/debug.o    #存储所有目标文件名字

##############     c代码编译     ###############
$(BUILD_DIR)/main.o: kernel/main.c lib/kernel/print.h \
        lib/stdint.h kernel/init.h
   $(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: kernel/init.c kernel/init.h lib/kernel/print.h \
        lib/stdint.h kernel/interrupt.h device/timer.h
   $(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c kernel/interrupt.h \
        lib/stdint.h kernel/global.h lib/kernel/io.h lib/kernel/print.h
   $(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o: device/timer.c device/timer.h lib/stdint.h\
         lib/kernel/io.h lib/kernel/print.h
   $(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/debug.o: kernel/debug.c kernel/debug.h \
        lib/kernel/print.h lib/stdint.h kernel/interrupt.h
   $(CC) $(CFLAGS) $< -o $@

##############    汇编代码编译    ###############
$(BUILD_DIR)/kernel.o: kernel/kernel.S
   $(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/print.o: lib/kernel/print.S
   $(AS) $(ASFLAGS) $< -o $@

##############    链接所有目标文件    #############
$(BUILD_DIR)/kernel.bin: $(OBJS)
   $(LD) $(LDFLAGS) $^ -o $@

.PHONY : mk_dir hd clean all

# ubantu中需要将dash修改为bash运行
# ls -al /bin/sh若结果为/bin/sh -> dash
# 执行sudo dpkg-reconfigure dash选择No即可
mk_dir:
   if [[ ! -d $(BUILD_DIR) ]];then mkdir $(BUILD_DIR);fi         #if语句来判断文件目录是否存在

hd:
   dd if=$(BUILD_DIR)/kernel.bin \
           of=/home/qdl/bochs-2.6.2/bin/hd60M.img \
           bs=512 count=200 seek=9 conv=notrunc

clean:
   cd $(BUILD_DIR) && rm -f ./*

build: $(BUILD_DIR)/kernel.bin

all: mk_dir build hd
```

为了调试方便我们新增加了断言(ASSERT)，其核心思想是若断言通过则什么都不做，若不通过则用循环实现等待，打印错误信息，具体内容见`debug.c`和`debug.h`，在`main.c`中对其进行测试

并且我们需要在`interrupt.h`和`interrupt.c`中添加内容，`interrupt.h`添加如下

```c
/* 定义中断的两种状态:
 * INTR_OFF值为0,表示关中断,
 * INTR_ON值为1,表示开中断 */
enum intr_status {     // 中断状态
    INTR_OFF,         // 中断关闭
    INTR_ON                // 中断打开
};

enum intr_status intr_get_status(void);
enum intr_status intr_set_status (enum intr_status);
enum intr_status intr_enable (void);
enum intr_status intr_disable (void);
```

下面是`interrupt.c`

```c
/* 开中断并返回开中断前的状态*/
enum intr_status intr_enable() {
   enum intr_status old_status;
   if (INTR_ON == intr_get_status()) {
      old_status = INTR_ON;
      return old_status;
   } else {
      old_status = INTR_OFF;
      asm volatile("sti");  // 开中断,sti指令将IF位置1
      return old_status;
   }
}

/* 关中断,并且返回关中断前的状态 */
enum intr_status intr_disable() {
   enum intr_status old_status;
   if (INTR_ON == intr_get_status()) {
      old_status = INTR_ON;
      asm volatile("cli" : : : "memory"); // 关中断,cli指令将IF位置0
      return old_status;
   } else {
      old_status = INTR_OFF;
      return old_status;
   }
}

/* 将中断状态设置为status */
enum intr_status intr_set_status(enum intr_status status) {
   return status & INTR_ON ? intr_enable() : intr_disable();
}

/* 获取当前中断状态 */
enum intr_status intr_get_status() {
   uint32_t eflags = 0;
   GET_EFLAGS(eflags);
   return (EFLAGS_IF & eflags) ? INTR_ON : INTR_OFF;
}
```

在`main.c`中对其测试

```c
#include "print.h"
#include "init.h"
#include "debug.h"
int main(void){
    put_str("Welcome to C7's kernel\n");
    init_all();
    ASSERT(1==2);
    while(1);
    return 0;
}
```

主目录下用`sudo make all`编译之后，测试断言运行效果如下所示

![image-20200826005225103](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200826005225103.png)

### 字符串函数实现

在lib目录下用`string.c`实现对字符串的一些操作函数，比较好理解就不多解释了，代码如下

```c
#include "string.h"
#include "global.h"
#include "debug.h"

/* 将dst_起始的size个字节置为value */
void memset(void* dst_, uint8_t value, uint32_t size) {
	ASSERT(dst_ != NULL);
	uint8_t* dst = (uint8_t*)dst_;
	while(size--)
	{
		*dst++ = value;
	}
}

/* 将src_起始的size个字节复制到dst_ */
void memcpy(void* dst_, const void* src_, uint32_t size) {
	ASSERT(dst_ != NULL && src_ != NULL);
	uint8_t* dst = (uint8_t*)dst_;
	const uint8_t* src = src_;
	while(size--)
	{
		*dst++ = *src++;
	}
}

/* 连续比较以地址a_和地址b_开头的size个字节,若相等则返回0,若a_大于b_返回+1,否则返回-1 */
int memcmp(const void* a_, const void* b_, uint32_t size) {
	const char* a = a_;
    const char* b = b_;
	ASSERT(a != NULL && b != NULL);
	
	while(size--) {
		if(*a != *b) {
			return *a > *b ? 1 : -1;
		}
		a++;
		b++;
	}
	return 0;
}

/* 将字符串从src_复制到dst_,'0'为截至条件 */
char* strcpy(char* dst_, const char* src_) {
	ASSERT(dst_ != NULL && src_ != NULL);
	char* r = dst_;		       // 用来返回目的字符串起始地址
	while((*dst_++ = *src_++));
	return r;
}

/* 返回字符串长度 */
uint32_t strlen(const char* str) {
	ASSERT(str != NULL);
	const char* p = str;
	while(*p++);
	return (p - str - 1);
}

/* 比较两个字符串,若a_中的字符大于b_中的字符返回1,相等时返回0,否则返回-1. */
int8_t strcmp (const char* a, const char* b) {
	ASSERT(a != NULL && b != NULL);
	while (*a != 0 && *a == *b) {
		a++;
		b++;
	}
/* 如果*a小于*b就返回-1,否则就属于*a大于等于*b的情况。在后面的布尔表达式"*a > *b"中,
 * 若*a大于*b,表达式就等于1,否则就表达式不成立,也就是布尔值为0,恰恰表示*a等于*b */
   return *a < *b ? -1 : *a > *b;
}

/* 从左到右查找字符串str中首次出现字符ch的地址(不是下标,是地址) */
char* strchr(const char* str, const uint8_t ch) {
	ASSERT(str != NULL);
	while(*str != 0)
	{
		if(*str == ch)
		{
			return (char*)str;	// 需要强制转化成和返回值类型一样,否则编译器会报const属性丢失,下同.
		}
		str++;
	}
	return NULL;
}

/* 从后往前查找字符串str中首次出现字符ch的地址(不是下标,是地址) */
char* strrchr(const char* str, const uint8_t ch) {
    ASSERT(str != NULL);
    const char* last_char = NULL;
    /* 从头到尾遍历一次,若存在ch字符,last_char总是该字符最后一次出现在串中的地址(不是下标,是地址)*/
    while (*str != 0) {
		if (*str == ch) {
			last_char = str;
		}
		str++;
	}
	return (char*)last_char;
}

/* 将字符串src_拼接到dst_后,将回拼接的串地址 */
char* strcat(char* dst_, const char* src_) {
    ASSERT(dst_ != NULL && src_ != NULL);
    char* str = dst_;
    while (*str++);
    --str;      // 别看错了，--str是独立的一句，并不是while的循环体
    while((*str++ = *src_++));	 // 当*str被赋值为0时,此时表达式不成立,正好添加了字符串结尾的0.
    return dst_;
}

/* 在字符串str中查找指定字符ch出现的次数 */
uint32_t strchrs(const char* str, uint8_t ch) {
	ASSERT(str != NULL);
	uint32_t ch_cnt = 0;
	const char* p = str;
	while(*p != 0) {
	if (*p == ch) {
		ch_cnt++;
    }
		p++;
   }
   return ch_cnt;
}
```

### BITMAP实现

位图用于实现资源管理，相当于一张表，表中为1表示占用，为0表示空闲，之后我们将其用来管理内存，我们在前面的基础之上实现BITMAP，在`lib/kernel`目录下新增`bitmap.h`与`bitmap.c`，代码如下，bitmap结构比较简单，只有两个成员：指针**bits**和位图的字节长度**btmp_bytes_len**

```c
#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_H
#include "global.h"
#define BITMAP_MASK 1
struct bitmap {
   uint32_t btmp_bytes_len;
/* 在遍历位图时,整体上以字节为单位,细节上是以位为单位,所以此处位图的指针必须是单字节 */
   uint8_t* bits;
};

void bitmap_init(struct bitmap* btmp);
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx);
int bitmap_scan(struct bitmap* btmp, uint32_t cnt);
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value);
#endif
```

下面的一些函数主要是对位图的一些操作函数，代码比较简单，其中较为核心的函数是`bitmap_scan`

```c
#include "bitmap.h"
#include "stdint.h"
#include "string.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"

/* 将位图btmp初始化 */
void bitmap_init(struct bitmap* btmp) {
   memset(btmp->bits, 0, btmp->btmp_bytes_len);   
}

/* 判断bit_idx位是否为1,若为1则返回true，否则返回false */
bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx) {
   uint32_t byte_idx = bit_idx / 8;    // 向下取整用于索引数组下标
   uint32_t bit_odd  = bit_idx % 8;    // 取余用于索引数组内的位
   return (btmp->bits[byte_idx] & (BITMAP_MASK << bit_odd));
}

/* 在位图中申请连续cnt个位,成功则返回其起始位下标，失败返回-1 */
int bitmap_scan(struct bitmap* btmp, uint32_t cnt) {
   uint32_t idx_byte = 0;	 // 用于记录空闲位所在的字节
/* 先逐字节比较,蛮力法 */
   while (( 0xff == btmp->bits[idx_byte]) && (idx_byte < btmp->btmp_bytes_len)) {
/* 1表示该位已分配,所以若为0xff,则表示该字节内已无空闲位,向下一字节继续找 */
      idx_byte++;
   }

   ASSERT(idx_byte < btmp->btmp_bytes_len);
   if (idx_byte == btmp->btmp_bytes_len) {  // 若该内存池找不到可用空间		
      return -1;
   }

 /* 若在位图数组范围内的某字节内找到了空闲位，
  * 在该字节内逐位比对,返回空闲位的索引。*/
   int idx_bit = 0;
 /* 和btmp->bits[idx_byte]这个字节逐位对比 */
   while ((uint8_t)(BITMAP_MASK << idx_bit) & btmp->bits[idx_byte]) { 
	 idx_bit++;
   }
	 
   int bit_idx_start = idx_byte * 8 + idx_bit;    // 空闲位在位图内的下标
   if (cnt == 1) {
      return bit_idx_start;
   }

   uint32_t bit_left = (btmp->btmp_bytes_len * 8 - bit_idx_start);   // 记录还有多少位可以判断
   uint32_t next_bit = bit_idx_start + 1;
   uint32_t count = 1;	      // 用于记录找到的空闲位的个数

   bit_idx_start = -1;	      // 先将其置为-1,若找不到连续的位就直接返回
   while (bit_left-- > 0) {
      if (!(bitmap_scan_test(btmp, next_bit))) {	 // 若next_bit为0
	 count++;
      } else {
	 count = 0;
      }
      if (count == cnt) {	    // 若找到连续的cnt个空位
	 bit_idx_start = next_bit - cnt + 1;
	 break;
      }
      next_bit++;          
   }
   return bit_idx_start;
}

/* 将位图btmp的bit_idx位设置为value */
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t value) {
   ASSERT((value == 0) || (value == 1));
   uint32_t byte_idx = bit_idx / 8;    // 向下取整用于索引数组下标
   uint32_t bit_odd  = bit_idx % 8;    // 取余用于索引数组内的位

/* 一般都会用个0x1这样的数对字节中的位操作,
 * 将1任意移动后再取反,或者先取反再移位,可用来对位置0操作。*/
   if (value) {		      // 如果value为1
      btmp->bits[byte_idx] |= (BITMAP_MASK << bit_odd);
   } else {		      // 若为0
      btmp->bits[byte_idx] &= ~(BITMAP_MASK << bit_odd);
   }
}
```

### 内存管理

根据之前的铺垫，为了实现内存中用户和内核的区分，我们用位图实现对内存使用情况的记录，我们将物理内存划分为用户内存池和内核内存池，一页为4KB大小。

内核在申请空间的时候，先从内核自己的虚拟地址池中分配好虚拟地址再从内核物理地址池中分配物理内存，最后在内核自己的页表中将这两种地址建立好映射关系，内存就分配完成。

对用户进程来说，它向操作系统申请内存时，操作系统先从用户进程自己的虚拟地址分配虚拟地址，在从用户物理内存池中分配空闲的物理内存，用户物理内存池是被所有用户进程所共享的。最后在用户进程自己的页表中将这两种地址建立好映射关系。

![image-20200827002745365](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200827002745365.png)

实现在kernel目录下新建`memory.c`和`memory.h`，虚拟内存池结构和物理内存池结构如下，物理内存多了一个记录大小的pool_size，因为虚拟地址是连续的4GB空间，相对而言空间非常大，而物理地址是有限的，所以不存在对虚拟地址大小的记录。

```c
struct virtual_addr
{
    struct bitmap vaddr_bitmap;
    uint32_t vaddr_start;
};

struct pool
{
    struct bitmap pool_bitmap;  // 内存池的位图结构
    uint32_t phy_addr_start; 
    uint32_t pool_size;         
};

struct pool kernel_pool, user_pool; // 生成内核内存池和用户内存池
struct virtual_addr kernel_vaddr;   // 此结构用来给内核分配虚拟地址
```

在前面创建页目录和页表的时候，我们将虚拟地址 `0xc0000000~0xc00fffff` 映射到了物理地址 `0x0~0xfffff`，0xc0000000 是内核空间的起始虚拟地址，这 1MB 空间做的对等映射。为了看起来使内存连续，所以这里内核堆空间的开始地址从 0xc0100000 开始，在之前的设计中，0xc009f000 为内核主线程的栈顶，0xc009e000 将作为主线程的 PCB 使用，那么在低端1MB的空间中，就只剩下`0xc009a000~0xc009dfff`这`4 * 4KB`的空间未使用，所以位图的地址就安排在 0xc009a000 处，这里还剩下四个页框的大小，所能表示的内存大小为512MB

```c
#define MEM_BITMAP_BASE 0xc009a000
#define K_HEAP_START 0xc0100000
```

关键初始化函数如下，主要实现对内核池与用户池在物理内存中的平均分配

```c
// 初始化内存池
static void mem_pool_init(uint32_t all_mem)
{
    put_str("   mem_pool_init start\n");

    // 目前只初始化了低端1MB的内存页表，也就是256个页表
    uint32_t page_table_size = PG_SIZE * 256;

    uint32_t used_mem = page_table_size + 0x100000;

    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PG_SIZE;

    uint16_t kernel_free_pages = all_free_pages / 2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;

    // 内核的位图大小，在位图中，1bit表示1页
    uint32_t kbm_length = kernel_free_pages / 8;
    uint32_t ubm_length = user_free_pages / 8;

    // 内核内存池的起始地址
    uint32_t kp_start = used_mem;

    // 用户内存池的起始地址
    uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;

    kernel_pool.phy_addr_start = kp_start;
    user_pool.phy_addr_start = up_start;

    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    user_pool.pool_size = user_free_pages * PG_SIZE;

    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

    kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
    user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length);

    // 输出内存信息
    put_str("      kernel_pool_bitmap_start:");
    put_int((int)kernel_pool.pool_bitmap.bits);
    put_str(" kernel_pool_phy_addr_start:");
    put_int(kernel_pool.phy_addr_start);
    put_str("\n");
    put_str("      user_pool_bitmap_start:");
    put_int((int)user_pool.pool_bitmap.bits);
    put_str(" user_pool_phy_addr_start:");
    put_int(user_pool.phy_addr_start);
    put_str("\n");

    // 将位图置0
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);

    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
    kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);

    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("   mem_pool_init done\n");
}

void mem_init()
{
    put_str("mem_init start\n");

    // 物理内存的大小放在地址0xb00处
    uint32_t mem_bytes_total = *((uint32_t*)0xb00);

    mem_pool_init(mem_bytes_total);

    put_str("mem_init done\n");
}
```

写入makefile文件，编译运行效果如下，我们还没有实现对任意内存申请的函数，这里只是先将内存池进行了初始化，内核物理内存池所用的位图地址在0xc009a000，内存池中第一块物理页地址是0x200000

![image-20200827231501451](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200827231501451.png)

接下来就是实现对内存的分配，首先复习一下32位虚拟地址的转换过程：

1. 高 10 位是页目录项 pde 的索引，用于在页目录表中定位 pde ，细节是处理器获取高 10 位后自动将其乘以 4，再加上页目录表的物理地址，这样便得到了 pde 索引对应的 pde 所在的物理地址，然后自动在该物理地址中，即该 pde 中，获取保存的页表物理地址。
2. 中间 10 位是页表项 pte 索引，用于在页表中定位 pte 。细节是处理器获取中间 10 位后自动将其乘以 4，再加上第一步中得到的页表的物理地址，这样便得到了 pte 索引对应的 pte 所在的物理地址，然后自动在该物理地址 (该 pte) 中获取保存的普通物理页的物理地址。
3. 低 12 位是物理页内的偏移 ，页大小是 4KB, 12 位可寻址的范围正好是 4KB，因此处理器便直接把低 12 位作为第二步中获取的物理页的偏移量，无需乘以 4。用物理页的物理地址加上这低 12 位的和便是这 32 位虚拟地址最终落向的物理地址。

比如访问虚拟地址`0x00c03123`，拆分步骤如下

```
0x00c03123 => 16进制
0000 0000 1100 0000 0011 0001 0010 0011 => 2进制
0000000011 0000000011 000100100011      => 重新组合为 10+10+12
pde 3      pte 3      偏移 123
```

整个过程如下图所示

![image-20200827231951955](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200827231951955.png)

32位地址在上面转换之后则落向物理地址，内存分配的过程：

1. 在虚拟内存池中申请n个虚拟页
2. 在物理内存池中分配物理页
3. 在页表中添加虚拟地址与物理地址的映射关系

接下来就是一步一步在`memory`文件中增加函数

**在虚拟内存池中申请n个虚拟页**

```c
/* 在pf表示的虚拟内存池中申请pg_cnt个虚拟页,
 * 成功则返回虚拟页的起始地址, 失败则返回NULL */
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
   int vaddr_start = 0, bit_idx_start = -1;
   uint32_t cnt = 0;
   if (pf == PF_KERNEL) { //若为内核内存池
      bit_idx_start  = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt); // 扫描虚拟地址池
      if (bit_idx_start == -1) { // 返回-1则退出
	 return NULL;
      }
      while(cnt < pg_cnt) {
	 bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1); // 循环逐位置一
      }
      vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE; // 将bit_idx_start转换为虚拟地址
   } else {	
   // 用户内存池,将来实现用户进程再补充
   }
   return (void*)vaddr_start; // 返回指针
}
```

**在物理内存池中分配物理页**

这个函数比较关键，主要是对位图的扫描和记录，然后根据位图索引返回分配的物理地址

```c
// 在m_pool指向的物理内存池中分配一个物理页
static void *palloc(struct pool *m_pool)
{
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
    if(bit_idx == -1)
    {
        return NULL;
    }

    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phyaddr = bit_idx * PG_SIZE + m_pool->phy_addr_start;

    return (void*)page_phyaddr;
}
```

**在页表中添加虚拟地址与物理地址的映射关系**

再次复习一下32位虚拟地址到物理地址的转换，我们后面实现pde和pte访问就是用的这个原理

1. 首先通过高10位的pde索引，找到页表的物理地址
2. 其次通过中间10位的pte索引，得到物理页的物理地址
3. 最后把低12位作为物理页的页内偏移，加上物理页的物理地址，即为最终的物理地址

下面是通过虚拟地址访问pte和pde的函数，利用页目录表中最后一项存储的是页目录表地址

```c
/* 得到虚拟地址vaddr对应的pte指针*/
uint32_t* pte_ptr(uint32_t vaddr) {
   /* 先访问到页表自己 + \
    * 再用页目录项pde(页目录内页表的索引)做为pte的索引访问到页表 + \
    * 再用pte的索引做为页内偏移*/
   uint32_t* pte = (uint32_t*)(0xffc00000 + \ // 最后一个页目录项保存的是页目录表物理地址，高十位指向最后一个页目录表项
                                              // 也就是第1023个pde，换算成十进制就是0x3ff再移到高10位就是0xffc00000
	 ((vaddr & 0xffc00000) >> 10) + \        
	 PTE_IDX(vaddr) * 4);
   return pte;
}

/* 得到虚拟地址vaddr对应的pde的指针 */
uint32_t* pde_ptr(uint32_t vaddr) {
   /* 0xfffff是用来访问到页表本身所在的地址 */
   uint32_t* pde = (uint32_t*)((0xfffff000) + PDE_IDX(vaddr) * 4);
   return pde;
}
```

在`m_pool`处申请物理页的函数

```c
/* 在m_pool指向的物理内存池中分配1个物理页,
 * 成功则返回页框的物理地址,失败则返回NULL */
static void* palloc(struct pool* m_pool) {
   /* 扫描或设置位图要保证原子操作 */
   int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);    // 找一个物理页面
   if (bit_idx == -1 ) {
      return NULL;
   }
   bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);	// 将此位bit_idx置1
   uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start); // page_phyaddr用于保存分配的物理页地址
   return (void*)page_phyaddr;
}
```

添加虚拟地址与物理地址的映射函数

```c
/* 页表中添加虚拟地址_vaddr与物理地址_page_phyaddr的映射 */
static void page_table_add(void* _vaddr, void* _page_phyaddr) {
   uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
   uint32_t* pde = pde_ptr(vaddr);
   uint32_t* pte = pte_ptr(vaddr);

/************************   注意   *************************
 * 执行*pte,会访问到空的pde。所以确保pde创建完成后才能执行*pte,
 * 否则会引发page_fault。因此在*pde为0时,*pte只能出现在下面else语句块中的*pde后面。
 * *********************************************************/
   /* 先在页目录内判断目录项的P位，若为1,则表示该表已存在 */
   if (*pde & 0x00000001) {	 // 页目录项和页表项的第0位为P,此处判断目录项是否存在
      ASSERT(!(*pte & 0x00000001));

      if (!(*pte & 0x00000001)) {   // 只要是创建页表,pte就应该不存在,多判断一下放心
	 *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);    // US=1,RW=1,P=1
      } else {			    //应该不会执行到这，因为上面的ASSERT会先执行。
	 PANIC("pte repeat");
	 *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);      // US=1,RW=1,P=1
      }
   } else {			    // 页目录项不存在,所以要先创建页目录再创建页表项.
      /* 页表中用到的页框一律从内核空间分配 */
      uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);

      *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);

      /* 分配到的物理页地址pde_phyaddr对应的物理内存清0,
       * 避免里面的陈旧数据变成了页表项,从而让页表混乱.
       * 访问到pde对应的物理地址,用pte取高20位便可.
       * 因为pte是基于该pde对应的物理地址内再寻址,
       * 把低12位置0便是该pde对应的物理页的起始*/
      memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);
         
      ASSERT(!(*pte & 0x00000001));
      *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);      // US=1,RW=1,P=1
   }
```

`malloc_page`函数负责申请虚拟地址并分配物理地址、建立映射，大致步骤如下

1. 通过vaddr_get在虚拟内存池中申请虚拟地址
2. 通过palloc在物理内存池中申请物理页
3. 通过page_table_add将以上两步得到的结果在页表中映射

```c
/* 分配pg_cnt个页空间,成功则返回起始虚拟地址,失败时返回NULL */
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
   ASSERT(pg_cnt > 0 && pg_cnt < 3840); //15MB来限制，pg_cnt < 15*1024*1024/4096 = 3840页
/***********   malloc_page的原理是三个动作的合成:   ***********
      1通过vaddr_get在虚拟内存池中申请虚拟地址
      2通过palloc在物理内存池中申请物理页
      3通过page_table_add将以上得到的虚拟地址和物理地址在页表中完成映射
***************************************************************/
   void* vaddr_start = vaddr_get(pf, pg_cnt);
   if (vaddr_start == NULL) {
      return NULL;
   }

   uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
   struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool; // 内核池还是用户池

   /* 因为虚拟地址是连续的,但物理地址可以是不连续的,所以逐个做映射*/
   while (cnt-- > 0) {
      void* page_phyaddr = palloc(mem_pool);
      if (page_phyaddr == NULL) {  // 失败时要将曾经已申请的虚拟地址和物理页全部回滚，在将来完成内存回收时再补充
	 return NULL;
      }
      page_table_add((void*)vaddr, page_phyaddr); // 在页表中做映射 
      vaddr += PG_SIZE;		 // 下一个虚拟页
   }
   return vaddr_start;
}
```

最后一个函数负责在物理内存池中申请pg_cnt页内存

```c
/* 从内核物理内存池中申请pg_cnt页内存,成功则返回其虚拟地址,失败则返回NULL */
void* get_kernel_pages(uint32_t pg_cnt) {
   void* vaddr =  malloc_page(PF_KERNEL, pg_cnt);
   if (vaddr != NULL) {	   // 若分配的地址不为空,将页框清0后返回
      memset(vaddr, 0, pg_cnt * PG_SIZE);
   }
   return vaddr;
}
```

最后我们在main.c中添加测试代码，申请三个页并打印其虚拟地址

```c
#include "print.h"
#include "init.h"
#include "memory.h"
int main(void) {
   put_str("Welcome to TJ's kernel\n");
   init_all();
   void* addr = get_kernel_pages(3);
   put_str("\n get_kernel_page start vaddr is ");
   put_int((uint32_t)addr);
   put_str("\n");
   while(1);
   return 0;
}
```

运行效果如下，期中最上面的红框表示**虚拟地址起始地址**，对照第二个红框的对应关系，第三个红框中为7是因为我们申请了三个页，第三位都为1，位图的变化和预期相符合。

![image-20200827231337153](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200827231337153.png)



