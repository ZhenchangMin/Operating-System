# Lec2: Different Views of OS
操作系统十分复杂，代码量庞大。
操作系统有三条主线：
‣ "软件 (应⽤)"
‣ "硬件 (计算机)"
‣ "操作系统 (软件直接访问硬件带来麻烦太多⽽引⼊的中间件)"
• 想要理解操作系统，对操作系统的服务对象 (应⽤程序) 有精确的理解是必不可少的。

## 应用视角
### 什么是程序
IDE简单好用，但是它隐藏了很多细节。
程序到底是如何运行的？到底发生了什么？
![image.png](image/6.png)
编译阶段，把源代码编译成可执行文件（二进制），然后放在磁盘上。
![image.png](image/7.png)
加载器把文件从磁盘加载到内存中，CPU从内存中取指令执行。
这个图片是linux下文件编译成可执行文件的过程，然后加载器把可执行文件加载到内存中，CPU从内存中取指令执行。

完整的编译流程：
```
hello.c → (cpp预处理) → hello.i → (gcc -S) → hello.s → (as汇编) → hello.o → (ld链接) → hello(可执行文件)
```

```c
#include <stdio.h>
int main() {
    printf("Hello, World!\n");
    return 0;
}
```
gcc 编译出来的⽂件⼀点也不⼩ (试试 gcc -static会更有惊喜)
• objdump ⼯具可以查看对应的汇编代码
• 但是⾥面似乎有太多我们"没想过的"东⻄？
• gcc到底做了啥？ --verbose选项

这么多东西，都是我们需要的吗？我们只需要打印一行helloworld就行了啊
gcc会链接一些库文件，来支持printf函数的实现，这些库文件包含了很多代码，所以编译出来的文件就很大了。

#### ELF文件结构与内存布局
可执行文件（ELF格式）主要包含：
- ELF header（文件头）
- 代码段（.text）：程序的机器指令
- 数据段（.data）：已初始化的全局变量
- 符号表（.symtab）：函数和变量名映射

程序加载进内存后的布局（Linux x86-64）：
- 内核内存（高地址，用户不可访问）
- 用户栈（从 2^48-1 向下增长）
- 共享库映射区
- 运行时堆（向上增长）
- 全局数据（.bss, .data）
- 只读代码段（从 0x400000 开始）
- 0 到 0x400000 被刻意不映射（捕获空指针等非法访问）

先删除printf，只留下int main() { }，编译出来的文件就小了很多。

可能会出现Segmentation fault(core dumped) 段错误的错误，访问了非法内存地址的时候就需要调试了。
调试工具 gdb 可以帮助我们调试程序，查看程序的运行状态，查看变量的值，查看函数调用栈等。

#### 用GDB观察程序执行
常用 GDB 命令：
```
starti        # 从第一条指令开始停止
layout asm    # 显示汇编窗口
info registers # 查看寄存器状态
p $sp         # 打印栈指针
x $sp         # 查看栈顶内存内容
si            # 单步执行一条机器指令
p $rip        # 打印当前指令地址
```

发现是pc=1的时候，出错了
为什么pc不能是1？
![image.png](image/8.png)
刻意留下来的地址，处理程序异常用的
所以从main函数直接return的话，程序就会跳转到这个地址，导致段错误。
只要不在main函数直接return，就不会出现这个问题了。
```c
int main(){
    while(1);
}
```
加入无限循环，就不会出现段错误了。

但是没办法结束程序了，必须强制结束。
如果没有操作系统，程序的指令只能作计算，没办法结束程序。

#### 程序指令的种类
程序的指令可以分为两类：
- **确定性的指令（Deterministic）**：如加减乘除、寄存器操作等，每次执行结果相同
- **非确定性的指令（Non-deterministic）**：如 syscall，结果取决于操作系统的响应，不可预测

解决异常退出：交给操作系统来处理异常退出，操作系统会捕捉到程序的异常退出，进行相应的处理，比如释放资源，记录日志等。
调用syscall系统调用来结束程序，操作系统会捕捉到系统调用。
![image.png](image/9.png)
然后进入内核态，操作系统会进行相应的处理，比如释放资源，记录日志等，然后返回用户态，程序就结束了。

```c
// 用syscall正确退出程序
#include <sys/syscall.h>
int main() {
    register int p1 asm("rax") = SYS_exit;
    register int p2 asm("rdi") = 1;
    asm("syscall");
}
```

#### Hello World 的极简实现（汇编）
不借助任何C库，只用syscall实现Hello World：
```asm
#include <sys/syscall.h>
.globl _start
_start:
  movq $SYS_write, %rax  // write(fd=1, buf=st, count=ed-st)
  movq $1, %rdi
  movq $st, %rsi
  movq $(ed - st), %rdx
  syscall
  movq $SYS_exit, %rax   // exit(1)
  movq $1, %rdi
  syscall
st:
  .ascii "\033[01;31mHello, OS World\033[0m\n"
ed:
```

回到什么是程序这个问题。
程序就是状态机（图灵机），程序的状态由寄存器和内存中的数据决定，程序的行为由指令决定。
对于（二进制）程序而言，状态=寄存器状态(R)+内存状态(M)
初始状态=操作系统加载程序时设置的寄存器状态和内存状态
状态转移=执行一条指令

![1773296159240](image/lec2viewsofOS/1773296159240.png)

特殊的指令syscall：把(M,R)交给操作系统，任其修改
CPU发现是syscall指令的时候，进入内核态，运行操作系统写的代码，操作系统可以修改寄存器状态和内存状态，然后返回用户态继续执行程序.
可以实现与操作系统中的其他对象交互
![1773296735404](image/lec2viewsofOS/1773296735404.png)
程序感知不到syscall的运行过程，只转接
这也是为什么操作系统可以看成是**更加高级的机器**

### 理解高级语言程序
高级语言本质上能描述的计算能力和汇编一致（都是人类能行计算，都逃不开图灵机的限制）
• 因此，事实上C语言写成的程序也可以看成是状态机模型
• 甚至不需要借助编译器，我们完全可以模仿汇编的单步执行
![1773296869760](image/lec2viewsofOS/1773296869760.png)

C程序的状态=堆+栈+全局变量，不需要深入到寄存器了
初始状态仅有一个frame 是main(argc, argv)，其他都是空的，全局变量为初始值
状态转移=执行一行C代码，执行当前调用栈栈顶的那个栈指针(frames.top.PC处)的简单语句
- 如果是函数调用的话，本质上就是压入一个新的栈帧(stack frame), 并设置这个新的frame.PC = 所调用函数的入口
- 函数返回 = pop frame

#### 两个状态机与编译器的桥梁
C程序和汇编程序是两个等价的状态机：
- `.s` (汇编/机器指令级状态机)：状态 = (M, R)，每条指令是一次状态迁移
- `.c` (C语言级状态机)：状态 = 堆+栈+全局变量，每条语句是一次迁移

**编译器就是两个状态机之间的桥梁**：
```
.s = compile(.c)
```
即编译器将 C 语言状态机"翻译"成汇编状态机，保证两者行为等价。

用 C 语言解释器的视角来理解程序执行：
```python
while(True):
    stmt = fetch_and_parse_statement()
    evaluate(stmt, environment)  # environment = program state (heap + stack + globals)
```

### 操作系统上的软件（应用程序）
• 可执行文件
‣ 与大家日常使用的文件 (hello.c, README.txt) 没有本质区别，都可以打开、读取、甚至改写
• 可执行文件的操作
‣ xxd 可以16进制数直接查看可执行文件
‣ vscode安装hex editor插件，可以直接编辑

![1773297541524](image/lec2viewsofOS/1773297541524.png)

**任何程序本质上都是调用 syscall 的状态机**
- 应用程序通过 syscall API 与操作系统中的对象交互
- 操作系统将所有程序纳入统一管理

常见的系统级工具程序（coreutils等）的本质也是 syscall 状态机：
- `ls`：调用 open/read/write 等文件系统 syscall
- `cat`：调用 open/read/write
- `cp`：open/read/write/close
- `shell`：fork/execve/wait

一个很重要的工具：strace
可以查看程序运行过程中调用了哪些系统调用(syscall)，以及系统调用的参数和返回值。
![1773297638370](image/lec2viewsofOS/1773297638370.png)

strace 常用选项：
```bash
strace ./a.out                # 追踪所有syscall
strace -e trace=%process ./a.out  # 只追踪进程管理相关syscall
strace -e trace=%file ./a.out     # 只追踪文件操作相关syscall
strace -e trace=%memory ./a.out   # 只追踪内存管理相关syscall
```

为什么我们能够"追踪"进程？因为所有进程都在"操作系统"的监控之下！
‣ 进程某种意义上是运行在"操作系统"这样的一个**虚拟机**中，而不是直面硬件，所以操作系统清楚进程的一切，也能修改其一切！
• 操作系统为进程提供了ptrace系统调用，其可以帮助一个进程去查看另外一个进程，甚至是修改另外进程的运行时的寄存器，以至于植入代码！
‣ Strace、GDB背后都是ptrace系统调用！
‣ 凭借这个系统调用，你们甚至可以实现自己的"动态"程序分析器！

### 操作系统中应用程序的"一生"
初始状态，执行execve系统调用，操作系统加载可执行文件到内存中，设置好寄存器状态和内存状态，然后进入用户态执行程序。
然后，状态机执行
- 进程管理：fork, execve, exit...
- 文件系统：open, read, write, close...
- 存储管理：mmap, munmap...
- exit退出

所有的这些程序都是在操作系统 API (syscall) 和操作系统中的对象上构建
‣ 本质都是**调用 syscall 的状态机**
‣ 对于这些应用而言，操作系统就是一个**帮助解释**这些syscall的存在而已，其和能够帮他们解释普通计算指令的CPU无异

## 硬件视角
硬件的核心是**数字电路**
状态=寄存器保存的值
初始状态=REST
迁移 = 组合逻辑电路(NAND, NOT, AND, OR, NOR…)计算寄存器下一时钟周期的值，每个时钟周期做一次迁移
![1773299297702](image/lec2viewsofOS/1773299297702.png)

计算机硬件状态机模型：
- **状态** = 内存 (M) + 寄存器 (R)
- **初始状态** = CPU Reset 后的状态
- **状态迁移** = 选择一个 CPU 执行 → 响应中断 → 从 PC 处取指令执行

为了让 "操作系统" 这个程序能够正确启动，计算机硬件系统必定和程序员之间存在约定：
‣ 首先就是 Reset 的状态。
‣ 然后是 Reset 以后执行的程序应该做什么

### 硬件系统和程序员的约定
![1773299840058](image/lec2viewsofOS/1773299840058.png)
![1773299858565](image/lec2viewsofOS/1773299858565.png)
![1773300022704](image/lec2viewsofOS/1773300022704.png)

**Bare-metal 约定（裸机约定）**：
- Firmware（固件）存放在 ROM 中，通过 Memory-mapped I/O 访问
- CPU Reset 后从固定地址开始执行 Firmware

#### x86 CPU Reset 状态
x86 CPU Reset 后的初始状态：
- `EIP = 0x0000FFF0`（CS 寄存器的 base 被设为 0xFFFF0000，所以第一条指令在物理地址 0xFFFFFFF0）
- `CR0 = 0x60000010`（实模式，分页关闭）
- `EFLAGS = 0x00000002`
- CPU 处于 16-bit 实模式，中断关闭

其他平台的 Reset Vector：
- MIPS：`0xBFC00000`
- ARM：`0x00000000`
- RISC-V：实现定义（implementation-defined）

#### BIOS vs UEFI
今天的 Firmware 面临麻烦得多的硬件： 指纹锁、USB 转接器上的 Linux-to-Go 优盘、USB 蓝牙转接器连接的蓝牙键盘、…
- 这些设备都需要 "驱动程序" 才能访问
- 而传统BIOS只能支持有限的硬件，因此需要UEFI来支持更多的硬件设备

**Legacy BIOS** 约定：
- 加载第一个可启动设备的前 512 字节到物理内存 `0x7C00`
- `CS:IP = 0x7C00`（即从该地址开始执行）
- 磁盘格式：MBR（Master Boot Record），最后两字节为魔数 `0x55AA`
- CPU 仍处于 16-bit 实模式

**UEFI** 标准化加载流程：
- 磁盘必须按 GPT（GUID Partition Table）方式格式化
- 预留一个 FAT32 分区（可用 `lsblk`/`fdisk` 查看）
- Firmware 能够加载任意大小的 PE 可执行文件 `.efi`
- EFI 应用可以再次返回 firmware

![1773300197779](image/lec2viewsofOS/1773300197779.png)

### CPU Reset后

CPU reset后，如何观察一个计算机系统的指令运行？
QEMU是一个开源的计算机模拟器，可以模拟各种计算机系统，包括x86、ARM、MIPS等。

#### 用 QEMU + GDB 观察 CPU Reset
```bash
# 启动 QEMU，等待 GDB 连接，暂停在第一条指令
qemu-system-x86_64 -s -S mbr.img

# 另开终端连接 GDB
gdb
target remote localhost:1234
# 此时 CPU 停在 Reset 后第一条指令，可以单步观察 Firmware 执行
```

![1773300639710](image/lec2viewsofOS/1773300639710.png)
![1773300651806](image/lec2viewsofOS/1773300651806.png)

#### Bootloader 与 GRUB
- MBR bootloader：位于磁盘第一个扇区（512字节），先在实模式运行
- GRUB（两阶段 Bootloader）：
  - Stage 1：MBR 中的小程序，加载 Stage 2
  - Stage 2：更大的加载器，支持文件系统，加载 Linux 内核
- Loader 工作流程：16-bit 实模式 → 进入 32-bit 保护模式 → 加载 ELF32/ELF64 内核镜像

#### 最小 OS 实现
从 CPU Reset 到 OS 运行的完整路径：
```
CPU RESET → Firmware(BIOS/UEFI) → MBR/EFI Bootloader → ELF image → 初始化C运行环境 → OS main()
```

Bare-metal C 代码要求：
- 静态链接（无动态库）
- Freestanding 模式（`-ffreestanding`，无标准库）
- 自己初始化 C 运行环境（清零 BSS 段、设置栈指针等）

#### AM（抽象机器，Abstract Machine）
不同硬件架构（x86, ARM, RISC-V...）差异很大，AM 提供统一的硬件抽象层：
- **TRM**（Turing Machine）：最基础的图灵机，提供内存、寄存器、I/O的基本访问
- **CTE**（Context/异常控制流扩展）：异常、中断处理
- **MPE**（多处理器扩展）
- **VME**（虚拟内存扩展）
- **I/O 扩展**

### 两种控制流
**正常控制流（Normal Control Flow）**：
- 冯·诺依曼指令循环：fetch → decode → execute，PC 按顺序递增或跳转
- 完全由程序自身的跳转指令控制

**异常控制流（Exceptional Control Flow, ECF）**：
- PC 的跳转不由正常的递增或分支控制
- 由"外部"事件强制触发（硬件中断、软件异常、陷阱指令）
- CPU 会跳转到预设的处理程序入口（中断向量表/中断描述符表中的地址）

中断处理机制：
- 实模式：IVT（中断向量表，位于 0x0000）
- 保护模式：IDT（中断描述符表，由 IDTR 寄存器指向）

#### 上下文切换（Context Switch）
当 ECF 发生时，CPU 在进入中断处理程序前必须保护现场（否则就回不来了！）

**上下文（Context）**：当前 CPU 的一些寄存器值（因为这些不像堆栈保存在内存中，一旦被赋予了其他值，旧值就"消失"了）。AM 统一将其抽象为 Context 结构体，大体包含：
- PC 寄存器（CS、IP）
- 栈指针（SP、SS）
- 控制寄存器
- Virtual address translation 入口寄存器
- 其他数据寄存器（EAX, EBX...）

**X86 上下文切换过程**：
1. 异常事件发生前：用户进程正常运行，CPU 寄存器保存当前状态
2. 异常发生后：CPU 自动将 SS:ESP, CS:EIP, EFLAGS, EAX, EBX 等保存到**内核栈**（安全可靠！）
3. 跳转到 handler() 执行中断处理程序
4. 中断处理完毕：将保存的现场从内核栈弹出到 CPU，恢复之前的执行

> **Context 给了我们多个 CPU 的虚像！是分时操作系统的核心。**

注意：中断处理时出现中断怎么办？→ 在中断处理期间 disable 中断（排队），处理结束后 enable。

#### 操作系统在硬件眼中
操作系统只能控制两个部分：
1. **自己的 main 函数**（经历 CPU reset → BIOS → MBR → C 环境初始化之后才最终到达的函数）
   - 一般用来初始化整个计算系统环境
2. **中断响应的函数**
   - 响应异步事件的指令流，会打断当前的正常指令流，强制转移到处理中断事件的指令流
   - 一般用来实现各种服务

**结论：在硬件眼中，操作系统就是个 C 程序，只是能直接访问计算机硬件（包括中断和 I/O 设备）。**

#### 异步事件处理
以 Ctrl+C 为例：
1. 键盘产生中断（hardware interrupt）
2. OS 捕捉到键盘中断 → 发送 SIGINT 信号给前台进程
3. 应用程序的 signal handler 被调用（ECF！）
4. 这就是**事件驱动编程**（Event-Driven Programming）的本质

## 抽象视角

### 我们还没真正理解操作系统
目前我们有两个视角，但都是从侧面：
- 应用的视角（自顶向下）：OS = 解释 syscall 的存在
- 硬件的视角（自底向上）：OS = 一个 C 程序，有完整硬件控制权

但操作系统太复杂，怎么从全局去看？——**抽象的视角：操作系统本身就是状态机！**

### 操作系统的"一生"

#### 初始化阶段
```
Bootloader 传递控制权
  → 初始化内存、处理器、I/O、存储设备、设置中断处理(IRQs)
  → 挂载 initrd，加载必要驱动，然后卸载 initrd
  → 挂载根文件系统
  → 启动第一个用户级进程：init（现代 Linux 为 systemd）
  → yield cpu（交出 CPU 控制权，进入事件驱动模式）
```
之后大量用户进程将从 init 开始。

#### 运行阶段
操作系统维护的内部状态：
- Meta(进程1), Meta(进程2)... ：进程控制表，存放进程id、进程栈指针、PC等
- Meta(文件1), Meta(文件2)...：文件描述符表
- Meta(地址1), Meta(地址2)...：虚拟内存映射
- 内核栈1, 内核栈2...：每个进程对应的内核栈
- 内核堆
- 内核代码区域

运行时的状态迁移（s → s' → s''）：
- 用户进程通过 **syscall API（trap）** 陷入内核 → OS 处理 → 返回用户空间
- 硬件中断发生（如磁盘读写完成）→ OS 响应中断 → 状态更新
- OS 的状态是**被动迁移**的：
  - 用户程序执行 syscall → 改变操作系统状态
  - 硬件中断事件发生（如时钟中断）→ 改变操作系统状态

### 操作系统是状态机
- **内部状态** = 用户进程的元信息 + 内核栈 + 内核堆 + 操作系统代码区
- 操作系统在硬件加载完毕和初始化之后就变成了 interrupt/trap/fault handler
- 操作系统的状态是**被动**迁移的

### 用代码实现一个抽象的操作系统（OS-Model）

我们真正关心的是：应用程序、系统调用（OS API）、操作系统内部实现。

一个 Toy 实现思路：
- 应用程序 = 纯粹计算的 Python 代码 + 系统调用
- 操作系统 = Python 系统调用实现，有"假想"的 I/O 设备

#### Toy OS 的四个系统调用 API
```python
sys_choose(xs)  # 返回 xs 中的一个随机选项（对应 rand）
sys_write(s)    # 输出字符串 s（对应 printf）
sys_spawn(fn)   # 创建一个可运行的状态机 fn（对应 pthread_create）
sys_sched()     # 随机切换到任意状态机执行（对应调度器）
```

#### 玩具系统编程示例
```python
count = 0

def Tprint(name):
    global count
    for i in range(3):
        count += 1
        sys_write(f'#{count:02} Hello from {name}{i+1}\n')
        sys_sched()

def main():
    n = sys_choose([3, 4, 5])
    sys_write(f'#Thread = {n}\n')
    for name in 'ABCDE'[:n]:
        sys_spawn(Tprint, name)
    sys_sched()
```

#### 实现系统调用
简单的系统调用实现：
```python
def sys_write(s): print(s)
def sys_choose(xs): return random.choice(xs)
def sys_spawn(t): runnables.append(t)
```

难点在于 `sys_sched()`——需要实现**上下文切换**（切换状态机）：
- 切换对于被切换的状态机必须是"透明"的（它不知道被切换走，也不知道被切换回来）
- 需要：**封存当前状态机的状态**（保护现场） + **恢复另一个"被封存"状态机的执行**

切换状态机是**"分时"操作系统的核心**。

#### 借用 Python 的语言机制：Generator（生成器）
Python 中什么对象可以被暂时终止运行，然后再被唤醒继续执行？——**Generator objects（生成器）**

`yield` 关键词可以暂停当前执行流，返回给 `next()` 的调用者；然后下一次调用 `next()` 时继续正常执行：
```python
def number(a):
    while True:
        a += 1
        yield a   # 暂停，把 a 产生给外界；等待 next() 再次唤醒

c = number(3)
next(c)  # → 4
next(c)  # → 5
```

还可以用 `send()` 向生成器传递信号，让其继续执行：
```python
def number(a):
    while True:
        a += 1
        a = yield a   # yield 既产生值，也接收外界传入的值

c = number(3)
next(c)      # → 4（启动生成器）
c.send(5)    # → 6（传入5，a=5，然后 a+=1=6）
c.send(10)   # → 11
# Send(None) 等价于 next()，直接恢复执行不传递信息
```

Python 帮助封存了 `yield` 处的执行环境，然后在下一次调用时切换回来——这就是上下文切换的本质！

#### 构建操作系统 Toy（os-model.py）

**关键点1**：利用系统构建的数据结构 `Thread` 来支撑目标程序运行：
```python
class Thread:
    """A "freezed" thread state."""
    def __init__(self, func, *args):
        self._func = func(*args)   # 根据目标程序创建生成器
        self.retval = None

    def step(self):
        """Proceed with the thread until its next trap."""
        syscall, args, *_ = self._func.send(self.retval)  # 运行该程序直到下个内核陷入
        self.retval = None
        return syscall, args
```

**关键点2**：操作系统初始化之后就变成了"事件"的处理者（这里简化为 syscall 的解释器）：
```python
def run(self):
    threads = [OperatingSystem.Thread(self._main)]  # 系统初始化工作！
    while threads:  # Any thread lives
        try:
            match (t := threads[0]).step():
                case 'choose', xs:   # Return a random choice
                    t.retval = random.choice(xs)
                case 'write', xs:    # Write to debug console
                    print(xs, end='')
                case 'spawn', (fn, args):  # Spawn a new thread
                    threads += [OperatingSystem.Thread(fn, *args)]
                case 'sched', _:     # Non-deterministic schedule
                    random.shuffle(threads)
        except StopIteration:  # A thread terminates
            threads.remove(t)
            random.shuffle(threads)  # sys_sched()
```
# ↑ interrupt/trap/fault 的 handler！

#### 更完整的 Toy（mosaic.py）

| mosaic系统调用 | Linux对应 | 作用 |
|---|---|---|
| sys_spawn(fn) | pthread_create | 创建从 fn 开始执行的线程 |
| sys_fork() | fork | 创建当前状态机的完整复制 |
| sys_sched() | 调度器 | 切换到随机的线程/进程执行 |
| sys_choose(xs) | rand | 返回一个 xs 中的随机选择 |
| sys_write(s) | printf | 向调试终端输出字符串 s |
| sys_bread(k) | read | 读取虚拟磁盘块 k 的数据 |
| sys_bwrite(k,v) | write | 向虚拟磁盘块 k 写入数据 v |
| sys_sync() | sync | 将所有向虚拟磁盘的数据写入落盘 |

### 总结：三个视角下的操作系统

| 视角 | 操作系统是什么 |
|---|---|
| 应用视角（自顶向下）| 帮助解释 syscall 的存在，比 CPU 更高级的"机器" |
| 硬件视角（自底向上）| 一个 C 程序，有完整的硬件控制权（中断、I/O） |
| 抽象视角（全局）| 一个被动迁移的状态机，初始化后变成 interrupt/trap/fault handler |
