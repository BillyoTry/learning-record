## 改进MBR

![image-20200811231946242.png](https://i.loli.net/2020/08/12/pTKt35mzMiFkc1Y.png)

把通过 BIOS 的输出改为通过显存

```
;主引导程序
;------------------------------------------------------------
SECTION MBR vstart=0x7c00
   mov ax,cs
   mov ds,ax
   mov es,ax
   mov ss,ax
   mov fs,ax
   mov sp,0x7c00
   mov ax,0xb800 ;参考上表的基址，此处是文本模式
   mov gs,ax

; 清屏
;利用0x06号功能，上卷全部行，则可清屏。
; -----------------------------------------------------------
;INT 0x10   功能号:0x06      功能描述:上卷窗口
;------------------------------------------------------
;输入：
;AH 功能号= 0x06
;AL = 上卷的行数(如果为0,表示全部)
;BH = 上卷行属性
;(CL,CH) = 窗口左上角的(X,Y)位置
;(DL,DH) = 窗口右下角的(X,Y)位置
;无返回值：
   mov     ax, 0600h
   mov     bx, 0700h
   mov     cx, 0               ; 左上角: (0, 0)
   mov     dx, 184fh          ; 右下角: (80,25),
                ; 因为VGA文本模式中，一行只能容纳80个字符,共25行。
                ; 下标从0开始，所以0x18=24,0x4f=79
   int     10h                 ; int 10h

   ; 输出背景色绿色，前景色红色，并且跳动的字符串"1 MBR"
   mov byte [gs:0x00],'2'      ; 一字节为数据,一字节为属性
   mov byte [gs:0x01],0xA4     ; A表示绿色背景闪烁，4表示前景色为红色

   mov byte [gs:0x02],' '
   mov byte [gs:0x03],0xA4

   mov byte [gs:0x04],'M'
   mov byte [gs:0x05],0xA4

   mov byte [gs:0x06],'B'
   mov byte [gs:0x07],0xA4

   mov byte [gs:0x08],'R'
   mov byte [gs:0x09],0xA4

   jmp $             ; 通过死循环使程序悬停在此

   times 510-($-$$) db 0
   db 0x55,0xaa
```

红色字体，绿色背景闪烁

![image-20200811233610260.png](https://i.loli.net/2020/08/12/FeVhfbOHtJ8qN6C.png)

### IO接口

- IO接口是连接CPU和硬件的桥梁，端口是IO接口开放给CPU的接口。

- in指令用于从端口中读取数据，其一般形式是：

  ```
  (1) in al,dx
  (2) in ax,dx
  其中al和ax用来存储从端口获取的数据，dx是指端口号。
  这是固定用法，只要用in指令，源操作数（端口号）必须是dx，而目的操作数是用al还是ax，取决于dx端口指代的寄存器是8位宽度还是16位宽度。
  ```

- out指令用于往端口中写数据，其一般形式是：

  ```
  (1)out dx,al
  (2)out dx,ax
  (3)out 立即数,al
  (4)out 立即数,ax
  注：in指令中源操作数是端口号，而out指令的目的操作数是端口号。
  out指令中，可以选用dx或立即数充当端口号。
  ```

### 硬盘控制器端口

![image-20200812170737747.png](https://i.loli.net/2020/08/12/YXnPcwt31osT8US.png)

- 端口可分为两组，Command Block registers和Control Block registers，Command Block registers用于向硬盘驱动器写入命令字或者从硬盘控制器获取硬盘状态，Control Block registers用于控制硬盘工作状态。

- data寄存器，其作用是读取或写入数据，在读硬盘时，硬盘准备好数据后，硬盘控制器将其放在内部的缓冲区中，不断读此寄存器便是读出缓冲区中的全部数据。在写硬盘时，我们要把数据源源不断地输送到此端口，数据便被存入缓冲区中，硬盘控制器发现这个缓冲区有数据了，便将此处的数据写入相应的扇区。

- 读硬盘时，端口0x171或0x1F1的寄存器名字叫Error寄存器，只在硬盘读取失败时才有用，里面才会记录失败的信息。在写硬盘时，此寄存器有了新的作用，改名叫Feature寄存器，有些命令需要指定额外的参数，这些参数就写在Feature寄存器中，注意：error和feature这两个名字指的是同一个寄存器，只是因为不用环境下有不同的用途，为了区分用途才有了不同的名字，他们都是8位寄存器。

- Sector count寄存器用来指定待读取或待写入的扇区数，硬盘每完成一个扇区，就会将此寄存器的值减1，如果中间失败了，此寄存器中的值便是尚未完成的扇区，他是8位寄存器，最大值为255，若指定为0，则表示要操作256个扇区。

- LBA寄存器有：LBA low、LBA mid、LBA high三个，他们三个都是8位宽的，low用来存储28位地址的第0～7位，mid用来存储第8～15位，high用来存储第16～23位。（磁盘中的扇区在物理上是用“柱面-磁头-扇区”（Cylinder Head Sector）来定位的，简称为CHS，他只是对于磁头来说很直观，因为它就是根据这些信息来定位扇区的，但是我们还是需要一套对人来说比较直观的寻址方式，即LBA（logical Block Address）全称为逻辑快地址--磁盘中扇区从0开始依次递增编号）。

- device寄存器，8位寄存器，低四位用来存储LBA地址的第24～27位，第四位用来指定通道上的主盘或从盘，0代表主盘，1代表从盘。第六位用来设置设置是否启用LBA方式，1代表LBA，0代表CHS。另外两位，第5位和第7位是固定为1的，称为MBS位。

  ![image-20200812170458170.png](https://i.loli.net/2020/08/12/8g6OdySwjGN4pCx.png)

- 在读硬盘时，端口0x1f7或0x177的寄存器名称是status，8位寄存器，用来给出硬盘的状态信息。第0位是ERR位，如果此位为1，表示出错，具体原因可以查看error寄存器。第三位是data request位，如果此位为1，表示硬盘已经把数据准备好。第六位是DRDY，表示硬盘就绪，此位是在硬盘诊断时使用的。第7位是BSY，表示硬盘是否繁忙。

  ![image-20200812170522113.png](https://i.loli.net/2020/08/12/VxGFUqYhC3WBLpw.png)

- 在写硬盘时，端口0x1F7或0x177的寄存器名称是command，和上面说过的error和feature寄存器情况一样，此寄存器用来存储让硬盘执行的命令。主要会用到：

  ```
  identify: 0xEC ,硬盘识别
  read sector: 0x20 ,读扇区
  write sector: 0x30 ,写扇区
  ```

### 常用的数据传送方式

1. 无条件传送方式
   - 应用此方式的数据源设备一定是随时准备好了数据，cpu可以随时取数据，如寄存器、内存就是类似这样的设备。
2. 查询传送方式
   - 也称为程序I/O、PIO（Programming Input/Output Model），是指传输之前，由程序先去检测设备的状态。数据源设备在一定条件下才能传送数据，这类设备通常是低速设备，比CPU慢很多，CPU需要数据时，先检查该设备的状态，如果准备就绪，CPU再去获取数据。硬盘有status寄存器，里面保存了工作状态，所以对硬盘采用这种方式。
3. 中断传送方式
   - 也称为中断驱动I/O。上面提到的“查询传送方式”存在缺陷，由于CPU需要不断的查询设备状态，所以导致了只有最后一刻的查询才是有意义的，之前的查询都是发生在数据尚未准备好的时间断里，导致了效率不高，仅对不要求速度的系统可以采用。采用中断传送方式，当数据源设备准备好数据后，他通过发中断来通知CPU拿数据，这样避免了CPU花在查询上的时间。
4. 直接存储器存取方式（DMA）
   - 上一个中断传送方式，虽然提高了CPU的利用率，但是由于中断，CPU需要通过压栈来保护现场，还要执行传输指令，最后还要恢复现场。如果采用了直接存储器存取方式就可以完全不消耗CPU资源，不让CPU参与传输，完全由数据源设备和内存直接传输。CPU直接到内存中拿数据就好了。不过该方法是由硬件实现的，不是软件概念，所以需要DMA控制器才行。
5. I/O处理机传送方式
   - DMA方式中CPU仍有一部分功能需要自己完成，比如数据交换、组合、校验等。于是又引入了另一个硬件--I/O处理机，有了它CPU可以完全不知道有数据传输这回事。

### 硬盘操作步骤

之前实现的mbr实际上并没有做什么事情，只是单纯的实现了和显卡交互，我们需要增加有实际用处的功能，而MBR只有512字节，没法实现对内核的加载，所以我们下一步需要让其怎加读写磁盘的功能，在磁盘中加载loader，然后用loader来实现对内核的加载。

MBR在第0扇区(逻辑LBA编号)，loader理论上可以在1扇区，这里为了安全起见放在2扇区，预留出1扇区的空位。MBR将2扇区的内容读出来，放入实模式1MB内存分布中的可用区域(参见BIOS处的表格)，因为loader中还会加载一些GDT等的描述符表，这些表不能被覆盖，随着内核越来越完整，loader的内核也不断从低地址向高地址发展，所以需要选择一个稍安全的地方，留出一些空位，这里选择0x900。大致步骤如下：

1. 先选择通道，往该通道的sector count寄存器中写入待操作的扇区数。
2. 往该通道上的三个LBA寄存器写入扇区起始地址的低24位。
3. 往device寄存器中写入LBA地址的24~27位，并置第6位为1，使其为LBA模式，设置第4位为，选择操作的硬盘(master硬盘或slave硬盘)。
4. 往该通道上的command寄存器写入操作命令。
5. 读取该通道上的status寄存器，判断硬盘工作是否完成。
6. 如果以上步骤是读硬盘，进入下一个步骤。否则，完工。
7. 将硬盘数据读出。

```
;主引导程序 
;------------------------------------------------------------
%include "boot.inc"
SECTION MBR vstart=0x7c00         
   mov ax,cs      
   mov ds,ax
   mov es,ax
   mov ss,ax
   mov fs,ax
   mov sp,0x7c00
   mov ax,0xb800
   mov gs,ax

;清屏
;利用0x06号功能，上卷全部行，则可清屏。
; -----------------------------------------------------------
;INT 0x10   功能号:0x06	   功能描述:上卷窗口
;------------------------------------------------------
;输入：
;AH 功能号= 0x06
;AL = 上卷的行数(如果为0,表示全部)
;BH = 上卷行属性
;(CL,CH) = 窗口左上角的(X,Y)位置
;(DL,DH) = 窗口右下角的(X,Y)位置
;无返回值：
   mov     ax, 0600h
   mov     bx, 0700h
   mov     cx, 0                   ; 左上角: (0, 0)
   mov     dx, 184fh		      ; 右下角: (80,25),
				   ; 因为VGA文本模式中，一行只能容纳80个字符,共25行。
				   ; 下标从0开始，所以0x18=24,0x4f=79
   int     10h                     ; int 10h

   ; 输出字符串:MBR
   mov byte [gs:0x00],'1'
   mov byte [gs:0x01],0xA4

   mov byte [gs:0x02],' '
   mov byte [gs:0x03],0xA4

   mov byte [gs:0x04],'M'
   mov byte [gs:0x05],0xA4	   ;A表示绿色背景闪烁，4表示前景色为红色

   mov byte [gs:0x06],'B'
   mov byte [gs:0x07],0xA4

   mov byte [gs:0x08],'R'
   mov byte [gs:0x09],0xA4
   ; 寄存器传三个参数
   mov eax,LOADER_START_SECTOR	 ; 起始扇区LBA地址
   mov bx,LOADER_BASE_ADDR       ; 写入的地址
   mov cx,1			            ; 待读入的扇区数,这里是简单的loader故一个扇区足够
   call rd_disk_m_16		    ; 以下读取程序的起始部分（一个扇区）
  
   jmp LOADER_BASE_ADDR
       
;-------------------------------------------------------------------------------
;功能:读取硬盘n个扇区
rd_disk_m_16:	   
;-------------------------------------------------------------------------------
				       ; eax=LBA扇区号
				       ; ebx=将数据写入的内存地址
				       ; ecx=读入的扇区数
      mov esi,eax	  ;备份eax
      mov di,cx		  ;备份cx
;读写硬盘:
;第1步：选择通道，往该通道的sector count寄存器中写入待操作的扇区数
;因为bochs配置文件中虚拟硬盘属于ata0,是Primary通道,所以sector count寄存器由0x1f2端口访问
      mov dx,0x1f2
      mov al,cl
      out dx,al            ;读取的扇区数
      ;out 往端口中写数据
      ;in  从端口中读数据

      mov eax,esi	   ;恢复ax

;第2步：将LBA地址写入三个LBA寄存器和device寄存器的低4位

      ;LBA地址7~0位写入端口0x1f3
      mov dx,0x1f3                       
      out dx,al                          

      ;LBA地址15~8位写入端口0x1f4
      mov cl,8
      shr eax,cl
      mov dx,0x1f4
      out dx,al

      ;LBA地址23~16位写入端口0x1f5
      shr eax,cl
      mov dx,0x1f5
      out dx,al

      shr eax,cl
      and al,0x0f	   ; lba第24~27位
      or al,0xe0	   ; 设置7～4位为1110,表示lba模式
      mov dx,0x1f6
      out dx,al

;第3步：向command寄存器写入读命令，0x20 
      mov dx,0x1f7 ;要写入的端口
      mov al,0x20  ;要写入的数据          
      out dx,al

;第4步：检测硬盘状态，读取该通道上的status寄存器，判断硬盘工作是否完成
  .not_ready:
      ;同一端口，写时表示写入命令字，读时表示读入硬盘状态
      nop
      in al,dx
      and al,0x88	       ;第3位为1表示硬盘控制器已准备好数据传输，第7位为1表示硬盘忙
      cmp al,0x08
      jnz .not_ready	   ;若未准备好，继续等。

;第5步：从0x1f0端口读数据
      mov ax, di
      mov dx, 256
      mul dx
      mov cx, ax	   ; di为要读取的扇区数，一个扇区有512字节，每次读入一个字，
			          ; 共需di*512/2次，所以di*256
      mov dx, 0x1f0
  .go_on_read: ; 循环写入bx指向的内存
      in ax,dx
      mov [bx],ax
      add bx,2		  
      loop .go_on_read
      ret

   times 510-($-$$) db 0
   db 0x55,0xaa
```

在 boot 目录下建立 了一个子目录 include，并把 `boot.inc` 放到了 include 目录下，我们需要在`boot.inc`中指定两句头文件参数，如下所示

```
LOADER_BASE_ADDR equ 0x900
LOADER_START_SECTOR equ 0x2  ;第2个扇区
```

```
nasm -I include/ -o 3mbr.bin 3mbr.S
dd if=/home/qdl/bochs-2.6.2/bin/mbr/boot/3mbr.bin of=/home/qdl/bochs-2.6.2/bin/hd60M.img bs=512 count=1  conv=notrunc
```

编译成功之后，发现我们还没有写loader，这会导致CPU跳转到`0x900`处的地方，所以下一步我们就需要实现一个简单的loader，至少保证能简单运行下去。复习一下现在位置我们所知道的开机流程：BIOS -> MBR -> Loader。

loader中的内容我们用之前MBR的即可，这里编译也是需要`sudo nasm -I include/ -o loader.bin loader.S`。

```
%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR

mov byte [gs:0x00],'2'
mov byte [gs:0x01],0xA4

mov byte [gs:0x02],' '
mov byte [gs:0x03],0xA4

mov byte [gs:0x04],'L'
mov byte [gs:0x05],0xA4

mov byte [gs:0x06],'O'
mov byte [gs:0x07],0xA4

mov byte [gs:0x08],'A'
mov byte [gs:0x09],0xA4

mov byte [gs:0x0a],'D'
mov byte [gs:0x0b],0xA4

mov byte [gs:0x0c],'E'
mov byte [gs:0x0d],0xA4

mov byte [gs:0x0e],'R'
mov byte [gs:0x0f],0xA4

jmp $
```

dd命令指定seek参数将其写入第2个扇区

```
sudo dd if=./loader.bin of=/home/qdl/bochs-2.6.2/bin/hd60M.img bs=512 count=1 seek=2 conv=notrunc
```

最后通过`sudo ./bochs -f bochsrc.disk`得到如下效果

![image-20200812205230373.png](https://i.loli.net/2020/08/12/mCc4Qdk5iKXhYgt.png)

实模式的安全缺陷总结：

1. 操作系统和用户属于同一特权级
2. 用户程序引用的地址都是指向真实的物理地址
3. 用户程序可以自由修改段基址，自由访问所有内存





