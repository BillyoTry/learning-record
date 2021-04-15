## 输入输出系统

### 同步机制-锁

思考之前代码的问题，字符打印问题主要出现在交界处无法打印正确，回忆put_str函数打印有三个步骤

- 获取光标值
- 将光标值转换为字节地址，在该地址处写入字符
- 更新光标值

在打印的时候，若线程A到了第二步，此时发生了时钟中断，那么线程B就会重新获取光标值，这样导致数据覆盖，所以我们需要保证公共资源显存只有一个线程访问，也就是需要保证原子性，我们需要在put_str函数中进行开关中断的操作，如下所示，后面对公共资源"光标寄存器"也需要这样进行原子操作避免GP异常，这样做可以正确的打印输出，但只能解决输出函数线程竞争的问题，如果其他地方也有这种竞争问题就需要我们用一种新的机制来解决，也就是锁的机制。

```c
   while(1) {
      intr_disable(); // 关中断
      put_str("...");
      intr_enable(); // 开中断
   };
```

要进行线程同步，肯定要在需要同步的地方阻止线程的切换。这里主要通过信号量的机制对公共资源加锁，达到同步的目的。信号量的原理本身比较简单。通过P、V操作来表示信号量的增减，如下。

P操作，减少信号量：

1. 判断信号量是否大于0
2. 如果大于0， 将其减一
3. 如果小于0，将当前线程阻塞

V操作，增加信号量：

1. 将信号量的值加一
2. 唤醒等待的线程

首先我们需要实现线程的阻塞与唤醒，阻塞通常是线程自己阻塞自己，唤醒通常是其他线程唤醒本线程。具体实现如下，在thread.c中进行修改

```c
/* 当前线程将自己阻塞,标志其状态为stat. */
void thread_block(enum task_status stat) 
{
    /* stat取值为TASK_BLOCKED,TASK_WAITING,TASK_HANGING,也就是只有这三种状态才不会被调度*/
    ASSERT(stat == TASK_BLOCKED || stat == TASK_WAITING || stat == TASK_HANGING);

    enum intr_status old_status = intr_disable();
    task_struct* cur_thread = running_thread();

    cur_thread->status = stat; // 置其状态为stat 
    schedule();            // 将当前线程换下处理器
    
    /* 待当前线程被解除阻塞后才继续运行下面的intr_set_status */
   intr_set_status(old_status);
}

/* 将线程pthread解除阻塞 */
void thread_unblock(struct task_struct* pthread) 
{
    enum intr_status old_status = intr_disable();
    ASSERT((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANGING));

    if (pthread->status != TASK_READY) 
    {
        ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));

        if (elem_find(&thread_ready_list, &pthread->general_tag)) 
        {
            PANIC("thread_unblock: blocked thread in ready_list\n");
        }   
        
        list_push(&thread_ready_list, &pthread->general_tag);    // 放到队列的最前面,使其尽快得到调度
        pthread->status = TASK_READY;
   }   
   intr_set_status(old_status);
}
```

信号量锁的结构如下，实现在thread/sync.c和.h，信号量仅仅是一个编程理念，实现功能即可

```c
/* 信号量结构 */
struct semaphore 
{
   uint8_t  value;
   struct   list waiters;
};

/* 锁结构 */
struct lock 
{
   task_struct* holder; // 锁的持有者
   struct   semaphore semaphore; // 用二元信号量实现锁
   uint32_t holder_repeat_nr; // 锁的持有者重复申请锁的次数
};
```

初始化就是给各个字段赋值

```c
/* 初始化信号量 */
void sema_init(struct semaphore* psema, uint8_t value) {
   psema->value = value;       // 为信号量赋初值
   list_init(&psema->waiters); //初始化信号量的等待队列
}

/* 初始化锁plock */
void lock_init(struct lock* plock) {
   plock->holder = NULL;
   plock->holder_repeat_nr = 0;
   sema_init(&plock->semaphore, 1);  // 信号量初值为1
}
```

P操作

```c
/* 信号量down操作 */
void sema_down(struct semaphore *psema)
{
    /* 关中断来保证原子操作 */
    enum intr_status old_status = intr_disable();
    while (psema->value == 0)
    { 
        // 若value为0,表示已经被别人持有
        ASSERT(!elem_find(&psema->waiters, &running_thread()->general_tag));
        /* 当前线程不应该已在信号量的waiters队列中 */
        if (elem_find(&psema->waiters, &running_thread()->general_tag))
        {
            PANIC("sema_down: thread blocked has been in waiters_list\n");
        }

        /* 若信号量的值等于0,则当前线程把自己加入该锁的等待队列,然后阻塞自己 */
        list_append(&psema->waiters, &running_thread()->general_tag);
        thread_block(TASK_BLOCKED); // 阻塞线程,直到被唤醒
    }
    /* 若value为1或被唤醒后,会执行下面的代码,也就是获得了锁。*/
    psema->value--;
    ASSERT(psema->value == 0);
    /* 恢复之前的中断状态 */
    intr_set_status(old_status);
}
```

V操作

```c
/* 信号量的up操作 */
void sema_up(struct semaphore *psema)
{
    /* 关中断,保证原子操作 */
    enum intr_status old_status = intr_disable();
    ASSERT(psema->value == 0);

    if (!list_empty(&psema->waiters))
    {
        task_struct *thread_blocked = elem2entry(task_struct, general_tag, list_pop(&psema->waiters));
        thread_unblock(thread_blocked);
    }

    psema->value++;
    ASSERT(psema->value == 1);
    /* 恢复之前的中断状态 */
    intr_set_status(old_status);
}
```

获取锁和释放锁

```c
/* 获取锁plock */
void lock_acquire(struct lock *plock)
{
    /* 排除曾经自己已经持有锁但还未将其释放的情况。*/
    if (plock->holder != running_thread()) // 排除死锁的情况
    {
        sema_down(&plock->semaphore); // 对信号量P操作,原子操作,信号量减一
        plock->holder = running_thread();
        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1;
    }
    else
    {
        plock->holder_repeat_nr++;
    }
}

/* 释放锁plock */
void lock_release(struct lock *plock)
{
    ASSERT(plock->holder == running_thread());
    if (plock->holder_repeat_nr > 1)
    {
        plock->holder_repeat_nr--;
        return;
    }
    ASSERT(plock->holder_repeat_nr == 1);

    plock->holder = NULL; // 把锁的持有者置空放在V操作之前
    plock->holder_repeat_nr = 0;
    sema_up(&plock->semaphore); // 信号量的V操作,也是原子操作
}
```

接下来需要对锁进行测试，我们需要对终端输出进行封装，基本上都是对锁的使用，没什么好说的

```c
#include "console.h"
#include "print.h"
#include "stdint.h"
#include "sync.h"
#include "thread.h"
static struct lock console_lock;    // 控制台锁

/* 初始化终端 */
void console_init() {
  lock_init(&console_lock); 
}

/* 获取终端 */
void console_acquire() {
   lock_acquire(&console_lock);
}

/* 释放终端 */
void console_release() {
   lock_release(&console_lock);
}

/* 终端中输出字符串 */
void console_put_str(char* str) {
   console_acquire(); 
   put_str(str); 
   console_release();
}

/* 终端中输出字符 */
void console_put_char(uint8_t char_asci) {
   console_acquire(); 
   put_char(char_asci); 
   console_release();
}

/* 终端中输出16进制整数 */
void console_put_int(uint32_t num) {
   console_acquire(); 
   put_int(num); 
   console_release();
}
```

然后在init文件添加初始化函数并在main文件进行测试，只需要将put_str("...")修改为console_put_str("...")，测试结果如下

![image-20200901214341345](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200901214341345.png)

### 键盘获取输入输出

键盘的输入和输出主要是对8042和8048芯片的操作，这两芯片的数据在书本中有介绍，主要是对端口0x60的操作，其作为IO缓冲区，关系如下

![image-20200901214552811](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200901214552811.png)

我们将键盘的输入根据键盘扫描码进行转换，最终需要将其转换为我们键盘按下字符对应的ASCII码。其本质就是，键盘中断处理程序负责接收按键信息，也就是扫描码，然后就是对扫描码的处理，我们将用驱动程序对其进行实现，需要分两个阶段完成

- 如果是一些用于操作方面的控制键，比如shift，crtl等，就交给键盘驱动中完成
- 如果是一些用于字符方面的键，就直接交给字符处理程序完成即可

我们在device/keyboard.c和.h中实现，其中对于操作控制键和其他键配合按下的情况，比如crtl+a这种就需要定义一个变量判断之前是否已经按下crtl键，对于shift组合字符我们用的是二维数组保存，如shift+1显示的是 ! 字符

```c
/* 定义以下变量记录相应键是否按下的状态,
 * ext_scancode用于记录makecode是否以0xe0开头 */
static bool ctrl_status, shift_status, alt_status, caps_lock_status, ext_scancode;

/* 以通码make_code为索引的二维数组 */
static char keymap[][2] = {
/* 扫描码   未与shift组合  与shift组合*/
/* ---------------------------------- */
/* 0x00 */	{0,	0},		
/* 0x01 */	{esc,	esc},		
/* 0x02 */	{'1',	'!'},		
/* 0x03 */	{'2',	'@'},		
/* 0x04 */	{'3',	'#'},		
/* 0x05 */	{'4',	'$'},		
/* 0x06 */	{'5',	'%'},		
/* 0x07 */	{'6',	'^'},		
/* 0x08 */	{'7',	'&'},		
/* 0x09 */	{'8',	'*'},		
/* 0x0A */	{'9',	'('},		
/* 0x0B */	{'0',	')'},		
/* 0x0C */	{'-',	'_'},		
/* 0x0D */	{'=',	'+'},		
/* 0x0E */	{backspace, backspace},	
/* 0x0F */	{tab,	tab},		
/* 0x10 */	{'q',	'Q'},		
/* 0x11 */	{'w',	'W'},		
/* 0x12 */	{'e',	'E'},		
/* 0x13 */	{'r',	'R'},		
/* 0x14 */	{'t',	'T'},		
/* 0x15 */	{'y',	'Y'},		
/* 0x16 */	{'u',	'U'},		
/* 0x17 */	{'i',	'I'},		
/* 0x18 */	{'o',	'O'},		
/* 0x19 */	{'p',	'P'},		
/* 0x1A */	{'[',	'{'},		
/* 0x1B */	{']',	'}'},		
/* 0x1C */	{enter,  enter},
/* 0x1D */	{ctrl_l_char, ctrl_l_char},
/* 0x1E */	{'a',	'A'},		
/* 0x1F */	{'s',	'S'},		
/* 0x20 */	{'d',	'D'},		
/* 0x21 */	{'f',	'F'},		
/* 0x22 */	{'g',	'G'},		
/* 0x23 */	{'h',	'H'},		
/* 0x24 */	{'j',	'J'},		
/* 0x25 */	{'k',	'K'},		
/* 0x26 */	{'l',	'L'},		
/* 0x27 */	{';',	':'},		
/* 0x28 */	{'\'',	'"'},		
/* 0x29 */	{'`',	'~'},		
/* 0x2A */	{shift_l_char, shift_l_char},	
/* 0x2B */	{'\\',	'|'},		
/* 0x2C */	{'z',	'Z'},		
/* 0x2D */	{'x',	'X'},		
/* 0x2E */	{'c',	'C'},		
/* 0x2F */	{'v',	'V'},		
/* 0x30 */	{'b',	'B'},		
/* 0x31 */	{'n',	'N'},		
/* 0x32 */	{'m',	'M'},		
/* 0x33 */	{',',	'<'},		
/* 0x34 */	{'.',	'>'},		
/* 0x35 */	{'/',	'?'},
/* 0x36	*/	{shift_r_char, shift_r_char},	
/* 0x37 */	{'*',	'*'},    	
/* 0x38 */	{alt_l_char, alt_l_char},
/* 0x39 */	{' ',	' '},		
/* 0x3A */	{caps_lock_char, caps_lock_char}
/*其它按键暂不处理*/
};
```

后面的函数都是对通码、断码、组合键的一些处理

```c
/* 键盘中断处理程序 */
static void intr_keyboard_handler(void) {

/* 这次中断发生前的上一次中断,以下任意三个键是否有按下 */
   bool ctrl_down_last = ctrl_status;	  
   bool shift_down_last = shift_status;
   bool caps_lock_last = caps_lock_status;

   bool break_code;
   uint16_t scancode = inb(KBD_BUF_PORT);

/* 若扫描码是e0开头的,表示此键的按下将产生多个扫描码,
 * 所以马上结束此次中断处理函数,等待下一个扫描码进来*/ 
   if (scancode == 0xe0) { 
      ext_scancode = true;    // 打开e0标记
      return;
   }

/* 如果上次是以0xe0开头,将扫描码合并 */
   if (ext_scancode) {
      scancode = ((0xe000) | scancode);
      ext_scancode = false;   // 关闭e0标记
   }   

   break_code = ((scancode & 0x0080) != 0);   // 获取break_code
   
   if (break_code) {   // 若是断码break_code(按键弹起时产生的扫描码)

   /* 由于ctrl_r 和alt_r的make_code和break_code都是两字节,
   所以可用下面的方法取make_code,多字节的扫描码暂不处理 */
      uint16_t make_code = (scancode &= 0xff7f);   // 得到其make_code(按键按下时产生的扫描码)

   /* 若是任意以下三个键弹起了,将状态置为false */
      if (make_code == ctrl_l_make || make_code == ctrl_r_make) { // crtl
	 ctrl_status = false;
      } else if (make_code == shift_l_make || make_code == shift_r_make) { // shift
	 shift_status = false;
      } else if (make_code == alt_l_make || make_code == alt_r_make) { // alt
	 alt_status = false;
      } /* 由于caps_lock不是弹起后关闭,所以需要单独处理 */

      return;   // 直接返回结束此次中断处理程序

   } 
   /* 若为通码,只处理数组中定义的键以及alt_right和ctrl键,全是make_code */
   else if ((scancode > 0x00 && scancode < 0x3b) || \
	       (scancode == alt_r_make) || \
	       (scancode == ctrl_r_make)) {
      bool shift = false;  // 判断是否与shift组合,用来在一维数组中索引对应的字符
      if ((scancode < 0x0e) || (scancode == 0x29) || \
	 (scancode == 0x1a) || (scancode == 0x1b) || \
	 (scancode == 0x2b) || (scancode == 0x27) || \
	 (scancode == 0x28) || (scancode == 0x33) || \
	 (scancode == 0x34) || (scancode == 0x35)) {  
	    /****** 代表两个字母的键 ********
		     0x0e 数字'0'~'9',字符'-',字符'='
		     0x29 字符'`'
		     0x1a 字符'['
		     0x1b 字符']'
		     0x2b 字符'\\'
		     0x27 字符';'
		     0x28 字符'\''
		     0x33 字符','
		     0x34 字符'.'
		     0x35 字符'/' 
	    *******************************/
	 if (shift_down_last) {  // 如果同时按下了shift键
	    shift = true;
	 }
      } else {	  // 默认为字母键
	 if (shift_down_last && caps_lock_last) {  // 如果shift和capslock同时按下
	    shift = false;
	 } else if (shift_down_last || caps_lock_last) { // 如果shift和capslock任意被按下
	    shift = true;
	 } else {
	    shift = false;
	 }
      }

      uint8_t index = (scancode &= 0x00ff);  // 将扫描码的高字节置0,主要是针对高字节是e0的扫描码.
      char cur_char = keymap[index][shift];  // 在数组中找到对应的字符
   
      /* 只处理ascii码不为0的键 */
      if (cur_char) {
	 put_char(cur_char);
	 return;
      }

      /* 记录本次是否按下了下面几类控制键之一,供下次键入时判断组合键 */
      if (scancode == ctrl_l_make || scancode == ctrl_r_make) {
	 ctrl_status = true;
      } else if (scancode == shift_l_make || scancode == shift_r_make) {
	 shift_status = true;
      } else if (scancode == alt_l_make || scancode == alt_r_make) {
	 alt_status = true;
      } else if (scancode == caps_lock_make) {
      /* 不管之前是否有按下caps_lock键,当再次按下时则状态取反,
       * 即:已经开启时,再按下同样的键是关闭。关闭时按下表示开启。*/
	 caps_lock_status = !caps_lock_status;
      }
   } else {
      put_str("unknown key\n");
   }
}
```

修改main函数对我们的输入进行测试

```c
#include "print.h"
#include "init.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"

void k_thread_a(void*);
void k_thread_b(void*);

int main(void) {
   put_str("Welcome to C7's kernel!\n");
   init_all();

//   thread_start("k_thread_a", 31, k_thread_a, "argA ");
//   thread_start("k_thread_b", 8, k_thread_b, "argB ");

   intr_enable();
   while(1); //{
      //console_put_str("Main ");
  // };
   return 0;
}

/* 在线程中运行的函数 */
void k_thread_a(void* arg) {     
/* 用void*来通用表示参数,被调用的函数知道自己需要什么类型的参数,自己转换再用 */
   char* para = arg;
   while(1) {
      console_put_str(para);
   }
}

/* 在线程中运行的函数 */
void k_thread_b(void* arg) {     
/* 用void*来通用表示参数,被调用的函数知道自己需要什么类型的参数,自己转换再用 */
   char* para = arg;
   while(1) {
      console_put_str(para);
   }
}
```

测试结果如下，可以实现大部分键盘的输入，但当使用小键盘中1~9的时候会显示未识别，不过这个问题不大

![image-20200902103347831](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200902103347831.png)

为了构建交互式的shell，我们需要实现一个缓冲区用来保存我们输入的指令，这里我们使用的是一个环形的缓冲区，既然是环形，就涉及到它的设计思路，我们使用的是生产者-消费者模型，具体实现在device目录下的ioqueue.c和.h文件中，其中队列结构如下所示

```c
#define bufsize 64

/* 环形队列 */
struct ioqueue {
// 生产者消费者问题
    struct lock lock;
 /* 生产者,缓冲区不满时就继续往里面放数据,
  * 否则就睡眠,此项记录哪个生产者在此缓冲区上睡眠。*/
    struct task_struct* producer;

 /* 消费者,缓冲区不空时就继续从往里面拿数据,
  * 否则就睡眠,此项记录哪个消费者在此缓冲区上睡眠。*/
    struct task_struct* consumer;
    char buf[bufsize];			    // 缓冲区大小
    int32_t head;			    // 队首,数据往队首处写入
    int32_t tail;			    // 队尾,数据从队尾处读出
};
```

```c
/* 初始化io队列ioq */
void ioqueue_init(struct ioqueue* ioq) {
   lock_init(&ioq->lock);     // 初始化io队列的锁
   ioq->producer = ioq->consumer = NULL;  // 生产者和消费者置空
   ioq->head = ioq->tail = 0; // 队列的首尾指针指向缓冲区数组第0个位置
}
```

其他函数如下所示，其中比较关键的是`ioq_getchar`和`ioq_putchar`函数

```c
/* 返回pos在缓冲区中的下一个位置值 */
static int32_t next_pos(int32_t pos) {
   return (pos + 1) % bufsize; 
}

/* 判断队列是否已满 */
bool ioq_full(struct ioqueue* ioq) {
   ASSERT(intr_get_status() == INTR_OFF);
   return next_pos(ioq->head) == ioq->tail;
}

/* 判断队列是否已空 */
static bool ioq_empty(struct ioqueue* ioq) {
   ASSERT(intr_get_status() == INTR_OFF);
   return ioq->head == ioq->tail;
}

/* 使当前生产者或消费者在此缓冲区上等待 */
static void ioq_wait(struct task_struct** waiter) {
   ASSERT(*waiter == NULL && waiter != NULL);
   *waiter = running_thread();
   thread_block(TASK_BLOCKED);
}

/* 唤醒waiter */
static void wakeup(struct task_struct** waiter) {
   ASSERT(*waiter != NULL);
   thread_unblock(*waiter); 
   *waiter = NULL;
}

/* 消费者从ioq队列中获取一个字符 */
char ioq_getchar(struct ioqueue* ioq) {
   ASSERT(intr_get_status() == INTR_OFF);

/* 若缓冲区(队列)为空,把消费者ioq->consumer记为当前线程自己,
 * 目的是将来生产者往缓冲区里装商品后,生产者知道唤醒哪个消费者,
 * 也就是唤醒当前线程自己*/
   while (ioq_empty(ioq)) {
      lock_acquire(&ioq->lock);	 
      ioq_wait(&ioq->consumer);
      lock_release(&ioq->lock);
   }

   char byte = ioq->buf[ioq->tail];	  // 从缓冲区中取出
   ioq->tail = next_pos(ioq->tail);	  // 把读游标移到下一位置

   if (ioq->producer != NULL) {
      wakeup(&ioq->producer);		  // 唤醒生产者
   }

   return byte; 
}

/* 生产者往ioq队列中写入一个字符byte */
void ioq_putchar(struct ioqueue* ioq, char byte) {
   ASSERT(intr_get_status() == INTR_OFF);

/* 若缓冲区(队列)已经满了,把生产者ioq->producer记为自己,
 * 为的是当缓冲区里的东西被消费者取完后让消费者知道唤醒哪个生产者,
 * 也就是唤醒当前线程自己*/
   while (ioq_full(ioq)) {
      lock_acquire(&ioq->lock);
      ioq_wait(&ioq->producer);
      lock_release(&ioq->lock);
   }
   ioq->buf[ioq->head] = byte;      // 把字节放入缓冲区中
   ioq->head = next_pos(ioq->head); // 把写游标移到下一位置

   if (ioq->consumer != NULL) {
      wakeup(&ioq->consumer);          // 唤醒消费者
   }
}
```

我们还需要修改interrupt.c文件，打开时钟中断和键盘中断，最后在main.c中修改测试代码如下

```c
[...]
int main(void) {
   put_str("Welcome to TJ's kernel\n");
   init_all();
   thread_start("consumer_a", 31, k_thread_a, " A_");
   thread_start("consumer_b", 31, k_thread_b, " B_");
   intr_enable();
   while(1); 
   return 0;
}

/* 在线程中运行的函数 */
void k_thread_a(void* arg) {     
   while(1) {
      enum intr_status old_status = intr_disable();
      if (!ioq_empty(&kbd_buf)) {
	 console_put_str(arg);
	 char byte = ioq_getchar(&kbd_buf);
	 console_put_char(byte);
      }
      intr_set_status(old_status);
   }
}

/* 在线程中运行的函数 */
void k_thread_b(void* arg) {     
   while(1) {
      enum intr_status old_status = intr_disable();
      if (!ioq_empty(&kbd_buf)) {
	 console_put_str(arg);
	 char byte = ioq_getchar(&kbd_buf);
	 console_put_char(byte);
      }
      intr_set_status(old_status);
   }
}
```

![image-20200902110324992](C:\Users\qin\AppData\Roaming\Typora\typora-user-images\image-20200902110324992.png)