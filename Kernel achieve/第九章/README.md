## 进程与线程

线程是具有能动性、执行力、独立性的代码块。进程 = 线程+资源。那么下面代码中你能区别普通函数和线程函数的区别么？我们知道普通的函数之间发生函数调用的时候，要进行压栈的一系列操作，然后调用，它需要依赖程序上下文的环境。而线程函数则是自己提供一套上下文环境，使其更加具有独立性的在处理器上执行。二者的区别也主要是上下文环境。

```c
void threadFunc(void *arg)
{
	printf("this is thread\n");
}

void func()
{
	printf("this is function\n");
}

int main()
{
	printf("this is main\n");
	
	_beginthread(threadFunc, 0, NULL);
	func();
	
	return 0;
}
```

我们再来说说进程，操作系统为每个进程提供了一个PCB，用于记录此进程相关的信息，**所有进程的PCB形成了一张表，这就是进程表，PCB又可以称为进程表项**。我们自己写的操作系统中PCB的结构是不固定的，其大致内容有寄存器映像、栈、pid、进程状态、优先级、父进程等，为了实现它，我们先创建thread目录，然后创建thread.c和.h文件，下面是PCB的结构，位于.h文件中

```c
/* 进程或线程的pcb,程序控制块 */
struct task_struct {
   uint32_t* self_kstack;	 // 各内核线程都用自己的内核栈
   enum task_status status;  // 记录线程状态
   uint8_t priority;		 // 线程优先级
   char name[16];
   uint32_t stack_magic;	 // 用这串数字做栈的边界标记,用于检测栈的溢出
};
```

我们的线程是在内核中实现的，所以申请PCB结构的时候是从内核池进行操作的，下面看看初始化的内容，主要内容是给PCB的各字段赋值

```c
/* 初始化线程基本信息 */
void init_thread(struct task_struct* pthread, char* name, int prio) {
   memset(pthread, 0, sizeof(*pthread));
   strcpy(pthread->name, name);
   pthread->status = TASK_RUNNING; 
   pthread->priority = prio;
/* self_kstack是线程自己在内核态下使用的栈顶地址 */
   pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
   pthread->stack_magic = 0x19870916;	  // 自定义的魔数
}
```

然后用`thread_create`初始化栈`thread_stack`，其中减去的操作主要是为了以后预留保存现场的空间

```c
/* 初始化线程栈thread_stack,将待执行的函数和参数放到thread_stack中相应的位置 */
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg) {
   /* 先预留中断使用栈的空间,可见thread.h中定义的结构 */
   pthread->self_kstack -= sizeof(struct intr_stack);

   /* 再留出线程栈空间,可见thread.h中定义 */
   pthread->self_kstack -= sizeof(struct thread_stack);
   struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
   kthread_stack->eip = kernel_thread;    //ret时，栈顶为kernel_thread函数地址
   kthread_stack->function = function;
   kthread_stack->func_arg = func_arg;
   kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}
```

上面的function即使线程所执行的函数，这个函数并不是用call去调用，我们用ret指令返回到`kernel_thread`后执行，CPU执行哪条指令是通过EIP的指向来决定的，而ret指令在返回的时候，当前的栈顶就会被当做是返回地址。也就是说，我们可以把某个函数的地址放在栈顶，通过这个函数来执行线程函数。那么在ret返回的时候，就会进入我们指定的函数当中，这个函数就会来调用线程函数。下面就是启动线程的函数

```c
/* 由kernel_thread去执行function(func_arg) */
static void kernel_thread(thread_func* function, void* func_arg) {
   function(func_arg); 
}

/* 创建一优先级为prio的线程,线程名为name,线程所执行的函数是function(func_arg) */
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg) {
/* pcb都位于内核空间,包括用户进程的pcb也是在内核空间 */
   struct task_struct* thread = get_kernel_pages(1);  // 申请一页内存用于放PCB

   init_thread(thread, name, prio);
   thread_create(thread, function, func_arg);
	
   // ret的时候栈顶为kernel_thread进而去执行该函数
   asm volatile ("movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret" : : "g" (thread->self_kstack) : "memory");
   return thread;
}
```

为了实验我们还需要在main.c中对thread_start进行调用

```c
#include "print.h"
#include "init.h"
#include "thread.h"

void k_thread_a(void*);

int main(void) {
   put_str("Welcome to C7's kernel!\n");
   init_all();

   thread_start("k_thread_a", 31, k_thread_a, "argA ");

   while(1);
   return 0;
}

/* 在线程中运行的函数 */
void k_thread_a(void* arg) {     
/* 用void*来通用表示参数,被调用的函数知道自己需要什么类型的参数,自己转换再用 */
   char* para = arg;
   while(1) {
      put_str(para);
   }
}
```

编译运行结果如下所示，测试成功

![image-20200828224805402](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200828224805402.png)

### 多线程调度

为了提高效率，实现多线程调度，我们需要用**数据结构对内核线程结构进行维护**，首先我们需要在lib/kernel目录下增加队列结构

```c
#include "list.h"
#include "interrupt.h"

/* 初始化双向链表list */
void list_init (struct list* list) {
   list->head.prev = NULL;
   list->head.next = &list->tail;
   list->tail.prev = &list->head;
   list->tail.next = NULL;
}

/* 把链表元素elem插入在元素before之前 */
void list_insert_before(struct list_elem* before, struct list_elem* elem) { 
   enum intr_status old_status = intr_disable(); // 对队列是原子操作，中断需要关闭

/* 将before前驱元素的后继元素更新为elem, 暂时使before脱离链表*/ 
   before->prev->next = elem; 

/* 更新elem自己的前驱结点为before的前驱,
 * 更新elem自己的后继结点为before, 于是before又回到链表 */
   elem->prev = before->prev;
   elem->next = before;

/* 更新before的前驱结点为elem */
   before->prev = elem;

   intr_set_status(old_status); // 恢复中断
}

/* 添加元素到列表队首,类似栈push操作 */
void list_push(struct list* plist, struct list_elem* elem) {
   list_insert_before(plist->head.next, elem); // 在队头插入elem
}

/* 追加元素到链表队尾,类似队列的先进先出操作 */
void list_append(struct list* plist, struct list_elem* elem) {
   list_insert_before(&plist->tail, elem);     // 在队尾的前面插入
}

/* 使元素pelem脱离链表 */
void list_remove(struct list_elem* pelem) {
   enum intr_status old_status = intr_disable();
   
   pelem->prev->next = pelem->next;
   pelem->next->prev = pelem->prev;

   intr_set_status(old_status);
}

/* 将链表第一个元素弹出并返回,类似栈的pop操作 */
struct list_elem* list_pop(struct list* plist) {
   struct list_elem* elem = plist->head.next;
   list_remove(elem);
   return elem;
} 

/* 从链表中查找元素obj_elem,成功时返回true,失败时返回false */
bool elem_find(struct list* plist, struct list_elem* obj_elem) {
   struct list_elem* elem = plist->head.next;
   while (elem != &plist->tail) {
      if (elem == obj_elem) {
	 return true;
      }
      elem = elem->next;
   }
   return false;
}

/* 把列表plist中的每个元素elem和arg传给回调函数func,
 * arg给func用来判断elem是否符合条件.
 * 本函数的功能是遍历列表内所有元素,逐个判断是否有符合条件的元素。
 * 找到符合条件的元素返回元素指针,否则返回NULL. */
struct list_elem* list_traversal(struct list* plist, function func, int arg) {
   struct list_elem* elem = plist->head.next;
/* 如果队列为空,就必然没有符合条件的结点,故直接返回NULL */
   if (list_empty(plist)) { 
      return NULL;
   }

   while (elem != &plist->tail) {
      if (func(elem, arg)) {		  // func返回ture则认为该元素在回调函数中符合条件,命中,故停止继续遍历
	 return elem;
      }					  // 若回调函数func返回true,则继续遍历
      elem = elem->next;	       
   }
   return NULL;
}

/* 返回链表长度 */
uint32_t list_len(struct list* plist) {
   struct list_elem* elem = plist->head.next;
   uint32_t length = 0;
   while (elem != &plist->tail) {
      length++; 
      elem = elem->next;
   }
   return length;
}

/* 判断链表是否为空,空时返回true,否则返回false */
bool list_empty(struct list* plist) {		// 判断队列是否为空
   return (plist->head.next == &plist->tail ? true : false);
}
```

多线程调度需要我们继续改进线程代码，我们用PCB中的general_tag字段作为节点链接所有PCB，其中还有一个ticks字段用于记录线程执行时间，ticks越大，优先级越高，时钟中断一次，ticks就会减一，当其为0的时候，调度器就会切换线程，选择另一个线程上处理器执行，然后打上TASK_RUNNING的标记，之后通过switch_to函数将新线程的寄存器环境恢复，这样新线程才得以执行，完整调度过程需要以下三步

- 时钟中断处理函数
- 调度器schedule
- 任务切换函数switch_to

调度器主要实现如下

```c
/* 实现任务调度 */
void schedule() {

   ASSERT(intr_get_status() == INTR_OFF);

   struct task_struct* cur = running_thread(); // 获取线程PCB
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
   switch_to(cur, next);
}
```

接下来是切换函数的实现，在thread/目录下创建switch.S，由两部分组成第一部分负责保存任务进入中断前的全部寄存器，第二部分负责保存esi、edi、ebx、ebp四个寄存器。堆栈图压栈之后的如下所示

![image-20200830103727705](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200830103727705.png)

代码如下所示

```
[bits 32]
section .text
global switch_to
switch_to:
   ;栈中此处是返回地址
   push esi
   push edi
   push ebx
   push ebp

   mov eax, [esp + 20]		 ; 得到栈中的参数cur, cur = [esp+20]
   mov [eax], esp                ; 保存栈顶指针esp. task_struct的self_kstack字段,
				 ; self_kstack在task_struct中的偏移为0,
				 ; 所以直接往thread开头处存4字节便可。
;------------------  以上是备份当前线程的环境，下面是恢复下一个线程的环境  ----------------
   mov eax, [esp + 24]		 ; 得到栈中的参数next, next = [esp+24]
   mov esp, [eax]		 ; pcb的第一个成员是self_kstack成员,用来记录0级栈顶指针,
				 ; 用来上cpu时恢复0级栈,0级栈中保存了进程或线程所有信息,包括3级栈指针
   pop ebp
   pop ebx
   pop edi
   pop esi
   ret				 ; 返回到上面switch_to下面的那句注释的返回地址,
				 ; 未由中断进入,第一次执行时会返回到kernel_thread
```

修改makefie、printf等一些文件之后，最终能实现多线程的调度主函数main.c如下所示

```c
#include "print.h"
#include "init.h"
#include "thread.h"
#include "interrupt.h"

void k_thread_a(void*);
void k_thread_b(void*);
int main(void) {
   put_str("Welcome to C7's kernel!\n");
   init_all();

   thread_start("k_thread_a", 31, k_thread_a, "argA ");
   thread_start("k_thread_b", 8, k_thread_b, "argB ");

   intr_enable();	// 打开中断,使时钟中断起作用
   while(1) {
      put_str("Main ");
   };
   return 0;
}

/* 在线程中运行的函数 */
void k_thread_a(void* arg) {     
/* 用void*来通用表示参数,被调用的函数知道自己需要什么类型的参数,自己转换再用 */
   char* para = arg;
   while(1) {
      put_str(para);
   }
}

/* 在线程中运行的函数 */
void k_thread_b(void* arg) {     
/* 用void*来通用表示参数,被调用的函数知道自己需要什么类型的参数,自己转换再用 */
   char* para = arg;
   while(1) {
      put_str(para);
   }
}
```

不过这里会引发GP异常，如下所示，可以用`nm build/kernel.bin | grep thread_start`查看线程函数地址，然后在线程函数下断点，再用`show exitint`打印中断信息，这样就可以观察异常处的寄存器信息，这里产生异常的原因是寄存器bx的值超过了段界限limit的值0x7fff

![image-20200829220502353](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200829220502353.png)

