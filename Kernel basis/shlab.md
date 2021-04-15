## Shell lab

### 实现函数

此lab主要实现一个shell，但是不用完全实现，只需补充`tsh.c`中的以下函数：

- `void eval(char *cmdline)`：解析并执行命令。
- `int builtin_cmd(char **argv)`：检测命令是否为内置命令`quit`、`fg`、`bg`、`jobs`。
- `void do_bgfg(char **argv)`：实现`bg`、`fg`命令。
- `void waitfg(pid_t pid)`：等待前台命令执行完成。
- `void sigchld_handler(int sig)`：处理`SIGCHLD`信号，即子进程停止或终止。
- `void sigint_handler(int sig)`：处理`SIGINT`信号，即来自键盘的中断`ctrl-c`。
- `void sigtstp_handler(int sig)`：处理`SIGTSTP`信号，即终端停止信号`ctrl-z`。

### 辅助函数

- `int parseline(const char *cmdline,char **argv)`：获取参数列表`char **argv`，返回是否为后台运行命令（`true`）。
- `void clearjob(struct job_t *job)`：清除`job`结构。
- `void initjobs(struct job_t *jobs)`：初始化`jobs`链表。
- `void maxjid(struct job_t *jobs)`：返回`jobs`链表中最大的`jid`号。
- `int addjob(struct job_t *jobs,pid_t pid,int state,char *cmdline)`：在`jobs`链表中添加`job`
- `int deletejob(struct job_t *jobs,pid_t pid)`：在`jobs`链表中删除`pid`的`job`。
- `pid_t fgpid(struct job_t *jobs)`：返回当前前台运行`job`的`pid`号。
- `struct job_t *getjobpid(struct job_t *jobs,pid_t pid)`：返回`pid`号的`job`。
- `struct job_t *getjobjid(struct job_t *jobs,int jid)`：返回`jid`号的`job`。
- `int pid2jid(pid_t pid)`：将`pid`号转化为`jid`。
- `void listjobs(struct job_t *jobs)`：打印`jobs`。
- `void sigquit_handler(int sig)`：处理`SIGQUIT`信号。

> 参考自LeyN的知乎文章

tsh内置命令：

```text
quit: 退出当前shell

jobs: 列出所有后台运行的工作

bg <job>: 这个命令将会向<job>代表的工作发送SIGCONT信号并放在后台运行，<job>可以是一个PID也可以是一个JID。

fg <job>: 这个命令会向<job>代表的工作发送SIGCONT信号并放在前台运行，<job>可以是一个PID也可以是一个JID。
```

### 具体实现

> 相关说明参考代码注释

#### `eval`

```c
void eval(char *cmdline) 
{
    char *argv[MAXARGS];
    char buf[MAXLINE];
    int bg;
    int state;
    pid_t pid;
    sigset_t mask_all,mask_one,prev;

    strcpy(buf,cmdline);
    bg=parseline(buf,argv);/*如果请求了BG job返回真，如果请求了FG job返回假*/

    //无参数则退出
    if(argv[0]==NULL){
        return;
    }

    //非内置命令`quit`、`fg`、`bg`、`jobs`
    if(!builtin_cmd(argv)){
        // 在函数内部加阻塞列表
        sigfillset(&mask_all);//把每个信号都添加到mask_all中
        sigemptyset(&mask_one);//初始化mask_one为空集合
        sigaddset(&mask_one, SIGCHLD);//将SIGCHLD信号添加到mask_one中

        // 为了避免父进程运行到addjob之前子进程就退出了，所以
        // 在fork子进程前阻塞sigchld信号，addjob后解除
        sigprocmask(SIG_SETMASK,&prev,NULL);
        if((pid==fork())==0){
            // 子进程继承了父进程的阻塞向量，也要解除阻塞，
            // 避免收不到它本身的子进程的信号
            sigprocmask(SIG_SETMASK, &prev, NULL);
            // 改进程组与自己pid一样
            if(setpgid(0,0)<0){
                perror("SETPGID ERROR");
                exit(0);
            }
            if (execve(argv[0], argv, environ) < 0){
                printf("%s: Command not found\n", argv[0]);
                exit(0);
            }
        }else{
            state=bg?BG:FG;
            // 依然是加塞，阻塞所有信号
            sigprocmask(SIG_BLOCK, &mask_all, NULL);
            addjob(jobs, pid, state, cmdline);
            sigprocmask(SIG_SETMASK, &prev, NULL);
        }
        //后台则打印，前台则等待子进程结束
        if(!bg){
            waitfg(pid);
        }else{
            printf("[%d] (%d) %s",pid2jid(pid), pid, cmdline);
        }
    }
    return;
}
```

#### `builtin_cmd`

```c
int builtin_cmd(char **argv) 
{
    //判断是否是内置命令，不是内置命令则返回0，如果是内置命令则返回1
    //我们的shell命令其实就是一个程序，而我们知道argv[0]存的是程序的名字
    //例如我们运行"quit"程序，那么argv[0]存的就是我们的程序名
    if(!strcmp(argv[0],"quit")){
        exit(0);    //  "quit"命令对应的行为
    }else if(!strcmp(argv[0],"jobs")){ //  "jobs"命令对应的行为
        listjobs(jobs);     // 打印`jobs`
        return 1;
    }else if(!strcmp(argv[0],"bg")||!strcmp(argv[0],"fg")){
        do_bgfg(argv);
        return 1;
    }
    //如果一个命令以“&”结尾，那么tsh应该将它们放在后台运行，否则就放在前台运行（并等待它的结束）
    //单独的&不处理
    else if(!strcmp(argv[0], "&")){
        return 1;
    }
    return 0;     /* not a builtin command */
}
```

#### `do_bgfg`

```c
void do_bgfg(char **argv) 
{
    //命令无参数
    if(argv[1]==NULL){
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }

    struct job_t*job;
    int id;

    // bg %5 和bg 5不一样,一个是对一个job操作，另一个是对进程操作，
    // 而job代表了一个进程组。

    //读到jid
    if(sscanf(argv[1],"%%%d",&id)>0){
        job=getjobjid(jobs,id);
        if(job==NULL){
            printf("%%%d: No such job\n", id);
            return ;
        }
    }
    //读到pid
    else if(sscanf(argv[1], "%d",&id)>0){
        job=getjobpid(jobs,id);
        if(job==NULL){
            printf("(%d): No such process\n", id);;
            return ;
        }
    }
    //格式错误
    else{
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        return;
    }
    //子进程单独成组，所以kill很方便
    if(!strcmp(argv[0],"bg")){
        //进程组是负数pid，发送信号并更改状态
        kill(-(job->pid), SIGCONT);//发送信号SIGCONT给进程组|pid|中的每个进程  kill函数相关用法参考csapp书籍
        job->state=BG;
        printf("[%d] (%d) %s",job->jid, job->pid, job->cmdline);
    }else{
        kill(-(job->pid), SIGCONT);
        job->state=FG;
        waitfg(job->pid);
    }
    return;
}
```

#### `waitfg`

```c
void waitfg(pid_t pid)
{
    // 进程回收不需要做，只要等待前台进程就行
    sigset_t mask_temp;
    sigemptyset(&mask_temp);//初始化mask_temp为空集合
    while (fgpid(jobs) > 0){//返回当前前台运行job的pid号
        // 设定不阻塞任何信号，用空集代替当前阻塞集合
        // 其实可以直接sleep显式等待信号
        sigsuspend(&mask_temp);//设置mask_temp代替当前阻塞集
    }
    return;
}
```

#### `sigchld_handler`

```c
void sigchld_handler(int sig) 
{
    int olderrno = errno;   // 保存旧errno
    pid_t pid;
    int status;
    sigset_t mask_all,prev;

    sigfillset(&mask_all);   //设置全阻塞
    while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0){
        //-1表示等待集合就是由父进程所有的子进程组成的
        // WNOHANG | WUNTRACED 是立即返回
        if (WIFEXITED(status)){  //如果子进程通过调用exit或者一个return正常终止，则返回真
            sigprocmask(SIG_BLOCK, &mask_all, &prev);//将mask_all中的值添加到block中
            deletejob(jobs,pid);
            sigprocmask(SIG_SETMASK, &prev, NULL);//block=prev,prev为mask_all添加到block前的值
        }else if(WIFSIGNALED(status)){ //如果子进程是因为一个未被捕获的信号终止的，那么返回真
            struct job_t* job=getjobpid(jobs,pid);
            sigprocmask(SIG_BLOCK, &mask_all, &prev);//将mask_all中的值添加到block中
            printf("Job [%d] (%d) terminated by signal %d\n", job->jid, job->pid, WTERMSIG(status));
            //WTERMSIG(status)为返回导致子进程终止的信号的编号
            deletejob(jobs,pid);
            sigprocmask(SIG_SETMASK, &prev, NULL);
        }else{
            struct job_t* job=getjobpid(jobs,pid);
            sigprocmask(SIG_BLOCK, &mask_all, &prev);
            printf("Job [%d] (%d) stopped by signal %d\n", job->jid, job->pid, WSTOPSIG(status));
            //WSTOPSIG(status)返回引起子进程停止的信号的编号
            job->state=ST;
            sigprocmask(SIG_SETMASK, &prev, NULL);
        }
    }
    errno=olderrno;
    return;
}
```

#### `sigint_handler`

```c
void sigint_handler(int sig) 
{
    //向子进程发送信号即可
    int olderrno=errno;
    pid_t pid=fgpid(jobs);
    if(pid!=0){
        kill(-pid,sig);
    }
    errno=olderrno;
    return;
}
```

#### `sigtstp_handler`

```c
void sigtstp_handler(int sig) 
{
    //向子进程发送信号即可
    int olderrno=errno;
    pit_t pid=fgpid(jobs);
    if(pid!=0){
        kill(-pid,sig);
    }
    errno=olderrno;
    return;
}
```