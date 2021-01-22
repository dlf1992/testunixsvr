#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include "commusvr.h"

using namespace std;
#define SOCKFILE "sock_file"

static void signal_action(int sig, siginfo_t* info, void* p)
{
	static int iSig11En = 1;
	struct sigaction act;
	sigset_t* mask = &act.sa_mask;
	int n = 0;
	printf("sig = %d",sig);
	if(11 == sig)
	{
		//printf("sig = 11 ........\n");
		pthread_exit((void*)syscall(SYS_gettid));
	 	sigwait(mask, &n);
		if (iSig11En)
		{
			iSig11En = 0;
		}
		else
		{
			//printf("return signal_action........\n");
			iSig11En = 1;
			return;
		}
	}
	else if((SIGUSR1==sig) || (SIGUSR2==sig))
	{
		//LogInfo("program exit");
		exit(0);
	}

	//printf("signal_action,sig = %d........\n",sig);	
}

static void block_bad_singals()
{
  	sigset_t   signal_mask;
	sigemptyset (&signal_mask);
	sigaddset (&signal_mask, SIGPIPE);
	
	if (pthread_sigmask (SIG_BLOCK, &signal_mask, NULL))
	{
    	//printf("block sigpipe error\n");
	}
}

void signal_Init(void)
{
	 struct sigaction act;

	 sigset_t* mask = &act.sa_mask;
	 act.sa_flags=SA_SIGINFO;     /** 设置SA_SIGINFO 表示传递附加信息到触发函数 **/
	 act.sa_sigaction=signal_action;
	 
	 block_bad_singals();
	 
	 // 在进行信号处理的时候屏蔽所有信号
	 sigemptyset(mask);   /** 清空阻塞信号 **/

	 //添加阻塞信号
	 sigaddset(mask, SIGABRT);
	 sigaddset(mask, SIGHUP);
	 sigaddset(mask, SIGQUIT);
	 sigaddset(mask, SIGILL);
	 sigaddset(mask, SIGTRAP);
	 sigaddset(mask, SIGIOT);
	 sigaddset(mask, SIGBUS);
	 sigaddset(mask, SIGFPE);
	 sigaddset(mask, SIGSEGV);
	 sigaddset(mask, SIGUSR1);
	 sigaddset(mask, SIGUSR2);

	 
	//安装信号处理函数
	 sigaction(SIGABRT,&act,NULL);
	 //sigaction(SIGEMT,&act,NULL);
	 sigaction(SIGHUP,&act,NULL);
	 sigaction(SIGQUIT,&act,NULL);
	 sigaction(SIGILL,&act,NULL);
	 sigaction(SIGTRAP,&act,NULL);
	 sigaction(SIGIOT,&act,NULL);
	 sigaction(SIGBUS,&act,NULL);
	 sigaction(SIGFPE,&act,NULL);
	 sigaction(SIGSEGV,&act,NULL);
	 sigaction(SIGUSR1,&act,NULL);
	 sigaction(SIGUSR2,&act,NULL);
	 /*
	  * linux重启或使用kill命令会向所有进程发送SIGTERM信号，所以不需要安装此信号的处理函数
	  */
	 sigaction(SIGINT,&act,NULL);

}
static int dealnotify(int fd, int type,int cmd)
{
	printf("eventnotify:fd = %d,type = %d,cmd = %d,",fd,type,cmd);
	if(cmd == 1)
	{
		printf("connect.\n");
	}
	else if(cmd == 2)
	{
		printf("disconnect.\n");
	}
	else
	{
		printf("\n");
	}
	return 0;

}
static void *worker(void *arg)
{
	StartUnixService(SOCKFILE,dealnotify);
	return NULL;
}
int main(int argc,char *argv[])
{	
	//信号初始化
	signal_Init();
	pthread_t tid;
	if(pthread_create(&tid, NULL, worker, NULL) != 0)
	{
		printf("thread worker creat error.\n");
		return -1;
	}
	while(1)
	{
		sleep(5);
	}	
	return 0;
}
