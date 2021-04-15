/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
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
        if(( pid==fork() ) == 0){
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

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
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

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
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

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
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

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
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

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
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

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
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

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}



