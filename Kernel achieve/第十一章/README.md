## 用户进程

### LDT

之前介绍GDT的时候提到过LDT，我们的操作系统本身不实现LDT，但其作用还是有必要了解的，LDT也叫局部描述符表。按照内存分段的方式，内存中的程序映像自然被分成了代码段、数据段等资源，这些资源属于程序私有部分，因此intel建议为每个程序单独赋予一个结构来存储其私有资源，这个结构就是LDT，因为是每个任务都有的，故其位置不固定，要找到它需要先像GDT那样注册，之后用选择子找到它。其格式如下，LDT中描述符的D位和L位固定为0，因为属于系统断描述符，因此S为0。描述符在S为0的前提下，若TYPE的值为0010，即表示描述符是LDT。与其配套的寄存器和指令即为LDTR和`lldt "16位通用寄存器" 或 "16位内存单元"`：

![image-20200902111813924](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200902111813924.png)

### TSS

单核CPU想要实现多任务，唯一的方案就是多个任务共享同一个CPU，也就是让CPU在多个任务间轮转。TSS就是给每个任务"关联"的一个任务状态段，用它来关联任务。TSS(任务状态段)是由程序员来提供，CPU进行维护。程序员提供是指需要我们定义一个结构体，里面存放任务要用的寄存器数据。CPU维护是指切换任务时，CPU会自动把旧任务的数据存放的结构体变量中，然后将新任务的TSS数据加载到相应的寄存器中。

TSS和之前所说的段一样，本质上也是一片存储数据的内存区域，CPU用这块内存区域保存任务的最新状态。所以也需要一个描述符结构来表示它，这个描述符就是TSS描述符，它的结构如下，因为属于系统断描述符，因此S为0。描述符在S为0的前提下，若TYPE的值为10B1，B位表示Busy，为1表示繁忙，0表示空闲

![image-20200902112723346](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200902112723346.png)

其工作模式和LDT相似，由**寄存器TR保存TSS的起始地址**，使用前也需要进行注册，都是通过选择子来访问的，将TSS加载到TR的指令是ltr，格式如下

```
ltr "16位通用寄存器" 或 "16位内存单元"
```

任务切换的方式有"中断+任务门"、"call或jmp+任务门"、和iretd三种方式，这些方式都比较繁琐，对于Linux系统以及大部分x86系统而言，这样使用TSS效率太低，这一套标准需要我们在"应付"的前提下达到最高效率，我们这里主要效仿Linux系统的做法，Linux为了提高任务切换的速度，通过如下方式来进行任务切换：

一个CPU上的所有任务共享一个TSS，通过TR寄存器保存这个TSS，在使用ltr指令加载TSS之后，该TR寄存器永远指向同一个TSS，之后在进行任务切换的时候也**不会重新加载TSS**，只需要把**TSS中的SS0和esp0更新**为新任务的内核栈的段地址及栈指针。

接下来我们实现TSS，在`kernel/global.h`中我们增加一些描述符属性

```c
// ----------------  GDT描述符属性  ----------------

#define	DESC_G_4K    1
#define	DESC_D_32    1
#define DESC_L	     0	// 64位代码标记，此处标记为0便可。
#define DESC_AVL     0	// cpu不用此位，暂置为0  
#define DESC_P	     1
#define DESC_DPL_0   0
#define DESC_DPL_1   1
#define DESC_DPL_2   2
#define DESC_DPL_3   3
/* 
   代码段和数据段属于存储段，tss和各种门描述符属于系统段
   s为1时表示存储段,为0时表示系统段.
*/
#define DESC_S_CODE	1
#define DESC_S_DATA	DESC_S_CODE
#define DESC_S_SYS	0
#define DESC_TYPE_CODE	8	// x=1,c=0,r=0,a=0 代码段是可执行的,非依从的,不可读的,已访问位a清0.  
#define DESC_TYPE_DATA  2	// x=0,e=0,w=1,a=0 数据段是不可执行的,向上扩展的,可写的,已访问位a清0.
#define DESC_TYPE_TSS   9	// B位为0,不忙

/* 第3个段描述符是显存,第4个是tss */
#define SELECTOR_U_CODE	   ((5 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_DATA	   ((6 << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_STACK   SELECTOR_U_DATA

#define GDT_ATTR_HIGH		 ((DESC_G_4K << 7) + (DESC_D_32 << 6) + (DESC_L << 5) + (DESC_AVL << 4))
#define GDT_CODE_ATTR_LOW_DPL3	 ((DESC_P << 7) + (DESC_DPL_3 << 5) + (DESC_S_CODE << 4) + DESC_TYPE_CODE)
#define GDT_DATA_ATTR_LOW_DPL3	 ((DESC_P << 7) + (DESC_DPL_3 << 5) + (DESC_S_DATA << 4) + DESC_TYPE_DATA)


//---------------  TSS描述符属性  ------------
#define TSS_DESC_D  0 

#define TSS_ATTR_HIGH ((DESC_G_4K << 7) + (TSS_DESC_D << 6) + (DESC_L << 5) + (DESC_AVL << 4) + 0x0)
#define TSS_ATTR_LOW ((DESC_P << 7) + (DESC_DPL_0 << 5) + (DESC_S_SYS << 4) + DESC_TYPE_TSS)
#define SELECTOR_TSS ((4 << 3) + (TI_GDT << 2 ) + RPL0)

struct gdt_desc {
   uint16_t limit_low_word;
   uint16_t base_low_word;
   uint8_t  base_mid_byte;
   uint8_t  attr_low_byte;
   uint8_t  limit_high_attr_high;
   uint8_t  base_high_byte;
}; 

#define PG_SIZE 4096
```

关键代码我们在`userprog/tss.c`中实现，首先根据tss结构构造如下结构体

``` c
/* 任务状态段tss结构 */
struct tss 
{
    uint32_t backlink;
    uint32_t* esp0;
    uint32_t ss0;
    uint32_t* esp1;
    uint32_t ss1;
    uint32_t* esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t (*eip) (void);
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint32_t trace;
    uint32_t io_base;
};
```

初始化主要是效仿Linux中初始化ss0和esp0，然后将TSS描述符加载到全局描述符表中，因为GDT中第0个描述符不可用，第1个为代码段，第2个为数据段和栈，第3个为显存段，第4个就是我们的tss，故地址为`0xc0000900+0x20`

```c
#include "tss.h"
#include "stdint.h"
#include "global.h"
#include "string.h"
#include "print.h"

/* 任务状态段tss结构 */
struct tss {
    uint32_t backlink;
    uint32_t* esp0;
    uint32_t ss0;
    uint32_t* esp1;
    uint32_t ss1;
    uint32_t* esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t (*eip) (void);
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint32_t trace;
    uint32_t io_base;
}; 
static struct tss tss;

/* 更新tss中esp0字段的值为pthread的0级线 */
void update_tss_esp(struct task_struct* pthread) {
   tss.esp0 = (uint32_t*)((uint32_t)pthread + PG_SIZE);
}

/* 创建gdt描述符 */
static struct gdt_desc make_gdt_desc(uint32_t* desc_addr, uint32_t limit, uint8_t attr_low, uint8_t attr_high) {
   uint32_t desc_base = (uint32_t)desc_addr;
   struct gdt_desc desc;
   desc.limit_low_word = limit & 0x0000ffff;
   desc.base_low_word = desc_base & 0x0000ffff;
   desc.base_mid_byte = ((desc_base & 0x00ff0000) >> 16);
   desc.attr_low_byte = (uint8_t)(attr_low);
   desc.limit_high_attr_high = (((limit & 0x000f0000) >> 16) + (uint8_t)(attr_high));
   desc.base_high_byte = desc_base >> 24;
   return desc;
}

/* 在gdt中创建tss并重新加载gdt */
void tss_init() {
   put_str("tss_init start\n");
   uint32_t tss_size = sizeof(tss);
   memset(&tss, 0, tss_size);
   tss.ss0 = SELECTOR_K_STACK;
   tss.io_base = tss_size;

/* gdt段基址为0x900,把tss放到第4个位置,也就是0x900+0x20的位置 */

  /* 在gdt中添加dpl为0的TSS描述符 */
  *((struct gdt_desc*)0xc0000920) = make_gdt_desc((uint32_t*)&tss, tss_size - 1, TSS_ATTR_LOW, TSS_ATTR_HIGH);

  /* 在gdt中添加dpl为3的数据段和代码段描述符 */
  *((struct gdt_desc*)0xc0000928) = make_gdt_desc((uint32_t*)0, 0xfffff, GDT_CODE_ATTR_LOW_DPL3, GDT_ATTR_HIGH);
  *((struct gdt_desc*)0xc0000930) = make_gdt_desc((uint32_t*)0, 0xfffff, GDT_DATA_ATTR_LOW_DPL3, GDT_ATTR_HIGH);
   
  /* gdt 16位的limit 32位的段基址 */
   uint64_t gdt_operand = ((8 * 7 - 1) | ((uint64_t)(uint32_t)0xc0000900 << 16));   // 7个描述符大小
   asm volatile ("lgdt %0" : : "m" (gdt_operand));  // 重新加载GDT
   asm volatile ("ltr %w0" : : "r" (SELECTOR_TSS)); // 加载tss到TR寄存器
   put_str("tss_init and ltr done\n");
}
```

修改初始化函数之后，测试一下，用info gdt命令查看gdt表，可以看到TSS正确加载到第四个描述符中。

![image-20200904210301053](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200904210301053.png)

![image-20200904210321100](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200904210321100.png)

### 进程实现

实现进程的过程是在之前的线程基础上进行的，在创建线程的时候是将栈的返回地址指向了kernel_thread函数，通过该函数调用线程函数实现的，其执行流程如下，我们**只需要把执行线程的函数换成创建进程的函数**就可以了

![image-20200904212011724](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200904212011724.png)

与线程不同的是，每个进程都单独有4GB虚拟地址空间，所以，需要单独为每个进程维护一个虚拟地址池，用来标记该进程中地址分配信息

```c
* 进程或线程的pcb,程序控制块 */
struct task_struct {
   uint32_t* self_kstack;	 // 各内核线程都用自己的内核栈
   enum task_status status;
   char name[16];
   uint8_t priority;
   uint8_t ticks;	   // 每次在处理器上执行的时间嘀嗒数

/* 此任务自上cpu运行后至今占用了多少cpu嘀嗒数,
 * 也就是此任务执行了多久*/
   uint32_t elapsed_ticks;

/* general_tag的作用是用于线程在一般的队列中的结点 */
   struct list_elem general_tag;				    

/* all_list_tag的作用是用于线程队列thread_all_list中的结点 */
   struct list_elem all_list_tag;

   uint32_t* pgdir;              // 进程自己页表的虚拟地址

   struct virtual_addr userprog_vaddr;   // 用户进程的虚拟地址
   uint32_t stack_magic;	 // 用这串数字做栈的边界标记,用于检测栈的溢出
};
```

用户进程创建页表的实现在memory.c中添加

```c
// 在虚拟内存池中申请pg_cnt个虚拟页
static void *vaddr_get(enum pool_flags pf, uint32_t pg_cnt)
{
    int vaddr_start = 0;
    int bit_idx_start = -1;
    uint32_t cnt = 0;

    if(pf == PF_KERNEL)
    {
        //...内核内存池
    }
    else
    {
        // 用户内存池
        task_struct *cur = running_thread();
        bit_idx_start = bitmap_scan(&cur->userprog_vaddr.vaddr_bitmap, pg_cnt);
        if(bit_idx_start == -1)
            return NULL;
        
        while (cnt < pg_cnt)
        {
            bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
        }

        vaddr_start = cur->userprog_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
        /* (0xc0000000 - PG_SIZE)做为用户3级栈已经在start_process被分配 */
        ASSERT((uint32_t)vaddr_start < (0xc0000000 - PG_SIZE));
    }

    return (void *)vaddr_start;
}

/* 在用户空间中申请4k内存,并返回其虚拟地址 */
void *get_user_pages(uint32_t pg_cnt)
{
    lock_acquire(&user_pool.lock);
    void *vaddr = malloc_page(PF_USER, pg_cnt);
    memset(vaddr, 0, pg_cnt * PG_SIZE);
    lock_release(&user_pool.lock);
    return vaddr;
}
```

我们还需让用户进程工作在3环境下，这就需要我们从高特权级跳到低特权级。一般情况下，CPU不允许从高特权级转向低特权级，只有从中断返回或者从调用门返回的情况下才可以。这里我们采用从中断返回的方式进入3特权级，需要制造从中断返回的条件，构造好栈的内容之后执行iretd指令，下面是添加的函数

```c
//构建用户进程初始上下文信息
void start_process(void *filename_)
{
    void *function = filename_;
    task_struct *cur = running_thread();
    cur->self_kstack += sizeof(thread_stack);
    intr_stack *proc_stack = (struct intr_stack *)cur->self_kstack;
    proc_stack->edi = proc_stack->esi = proc_stack->ebp = proc_stack->esp_dummy = 0;
    proc_stack->ebx = proc_stack->edx = proc_stack->ecx = proc_stack->eax = 0;
    proc_stack->gs = 0; // 用户态用不上,直接初始为0
    proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
    proc_stack->eip = function; // 待执行的用户程序地址
    proc_stack->cs = SELECTOR_U_CODE;
    proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    proc_stack->esp = (void *)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE);
    proc_stack->ss = SELECTOR_U_DATA;
    asm volatile("movl %0, %%esp; jmp intr_exit"
                 :
                 : "g"(proc_stack)
                 : "memory");
}
```

激活页表，其参数可能是进程也可能是线程

```c
/* 激活页表 */
void page_dir_activate(task_struct *p_thread)
{
    /* 若为内核线程,需要重新填充页表为0x100000 */
    uint32_t pagedir_phy_addr = 0x100000; // 默认为内核的页目录物理地址,也就是内核线程所用的页目录表
    if (p_thread->pgdir != NULL) // 用户态进程有页表，线程为NULL
    { // 用户态进程有自己的页目录表
        pagedir_phy_addr = addr_v2p((uint32_t)p_thread->pgdir);
    }

    /* 更新页目录寄存器cr3,使新页表生效 */
    asm volatile("movl %0, %%cr3"
                 :
                 : "r"(pagedir_phy_addr)
                 : "memory");
}

/* 激活线程或进程的页表,更新tss中的esp0为进程的特权级0的栈 */
void process_activate(task_struct *p_thread)
{
    ASSERT(p_thread != NULL);
    /* 击活该进程或线程的页表 */
    page_dir_activate(p_thread);

    /* 内核线程特权级本身就是0,处理器进入中断时并不会从tss中获取0特权级栈地址,故不需要更新esp0 */
    if (p_thread->pgdir)
    {
        /* 更新该进程的esp0,用于此进程被中断时保留上下文 */
        update_tss_esp(p_thread);
    }
}
```

创建用户进程的页目录表

```c
uint32_t *create_page_dir(void)
{
    /* 用户进程的页表不能让用户直接访问到,所以在内核空间来申请 */
    uint32_t *page_dir_vaddr = get_kernel_pages(1);
    if (page_dir_vaddr == NULL)
    {
        console_put_str("create_page_dir: get_kernel_page failed!");
        return NULL;
    }

    /************************** 1  先复制页表  *************************************/
    /*  page_dir_vaddr + 0x300*4 是内核页目录的第768项 */
    // 内核页目录项复制到用户进程使用的页目录项中
    memcpy((uint32_t *)((uint32_t)page_dir_vaddr + 0x300 * 4), (uint32_t *)(0xfffff000 + 0x300 * 4), 1024);
    /*****************************************************************************/

    /************************** 2  更新页目录地址 **********************************/
    uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr);
    /* 页目录地址是存入在页目录的最后一项,更新页目录地址为新页目录的物理地址 */
    page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
    /*****************************************************************************/
    return page_dir_vaddr;
}

/* 创建用户进程虚拟地址位图 */
void create_user_vaddr_bitmap(task_struct *user_prog)
{
    user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8, PG_SIZE);
    user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);
    user_prog->userprog_vaddr.vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
    bitmap_init(&user_prog->userprog_vaddr.vaddr_bitmap);
}
```

创建用户进程filename并将其添加到就绪队列中

```c
/* 创建用户进程 */
void process_execute(void *filename, char *name)
{
    /* pcb内核的数据结构,由内核来维护进程信息,因此要在内核内存池中申请 */
    task_struct *thread = get_kernel_pages(1);
    init_thread(thread, name, default_prio);
    create_user_vaddr_bitmap(thread);
    thread_create(thread, start_process, filename);
    thread->pgdir = create_page_dir();

    enum intr_status old_status = intr_disable();
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);

    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);
    intr_set_status(old_status);
}
```

要执行用户进程，我们需要通过调度器将其调度，不过这里因为用户进程是ring3，内核线程是ring0，故我们需要修改调度器

```c
/* 实现任务调度 */
void schedule() {

   ASSERT(intr_get_status() == INTR_OFF);

   struct task_struct* cur = running_thread(); 
   if (cur->status == TASK_RUNNING) { // 若此线程只是cpu时间片到了,将其加入到就绪队列尾
      ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
      list_append(&thread_ready_list, &cur->general_tag);
      cur->ticks = cur->priority;     // 重新将当前线程的ticks再重置为其priority;
      cur->status = TASK_READY;
   } else { 
      /* 若此线程需要某事件发生后才能继续上cpu运行,
      不需要将其加入队列,因为当前线程不在就绪队列中。*/
   }

   ASSERT(!list_empty(&thread_ready_list));
   thread_tag = NULL;	  // thread_tag清空
/* 将thread_ready_list队列中的第一个就绪线程弹出,准备将其调度上cpu. */
   thread_tag = list_pop(&thread_ready_list);   
   struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
   next->status = TASK_RUNNING;

   /* 击活任务页表等 */
   process_activate(next);

   switch_to(cur, next);
}
```

最后在main中添加测试代码，用内核线程帮进程打印数据

```c
[...]
void k_thread_a(void*);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);
int test_var_a = 0, test_var_b = 0;

int main(void) {
   put_str("Welcome to C7's kernel\n");
   init_all();

   thread_start("k_thread_a", 31, k_thread_a, "argA ");
   thread_start("k_thread_b", 31, k_thread_b, "argB ");
   process_execute(u_prog_a, "user_prog_a");
   process_execute(u_prog_b, "user_prog_b");

   intr_enable();
   while(1);
   return 0;
}

/* 在线程中运行的函数 */
void k_thread_a(void* arg) {     
   char* para = arg;
   while(1) {
      console_put_str(" v_a:0x");
      console_put_int(test_var_a);
   }
}

/* 在线程中运行的函数 */
void k_thread_b(void* arg) {     
   char* para = arg;
   while(1) {
      console_put_str(" v_b:0x");
      console_put_int(test_var_b);
   }
}

/* 测试用户进程 */
void u_prog_a(void) {
   while(1) {
      test_var_a++;
   }
}

/* 测试用户进程 */
void u_prog_b(void) {
   while(1) {
      test_var_b++;
   }
}
```

测试结果如下所示，在u_prog_a进程下断点观察cs为0x002b，和预期相符

![image-20200904235328925](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200904235328925.png)

![image-20200904235339558](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200904235339558.png)