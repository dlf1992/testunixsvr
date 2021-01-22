/******************************************************************
* Copyright(c) 2020-2028 ZHENGZHOU Tiamaes LTD.
* All right reserved. See COPYRIGHT for detailed Information.
*
* @fileName: taskprocess.h
* @brief: 域通信服务端数据任务处理
* @author: dinglf
* @date: 2020-05-06
* @history:
*******************************************************************/
#ifndef _TASK_PROCESS_
#define _TASK_PROCESS_

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <map>
#include <sys/time.h>

#include "locker.h"
#include "RingBuffer.h"

using namespace std;
#define MAX_RingBuffer 100
#define MAX_BUFFER 2048  //Buffer的最大字节

typedef  int (*pNotifyFun)(int,int,int);//fd type cmd

typedef map<int,int> clienttypemap;//fd type 
typedef map<int,TRingBuffer*> ringbuffermap;
typedef map<TRingBuffer*,mutex_locker*> lockermap;

class BaseTask
{
public:
	BaseTask(){}
	virtual ~BaseTask(){}
	virtual void doit() = 0;
};
 
class Task : public BaseTask
{
private:
	int sockfd;
	char order[MAX_BUFFER];
	int orderlen;
public:
	static clienttypemap m_clienttype; //存储客户端fd、类型	
	static TRingBuffer *pTRingBuffer[MAX_RingBuffer]; //环形队列指针数组
	static mutex_locker *pClientlocker[MAX_RingBuffer]; //锁每个客户端的环形队列
	static ringbuffermap map_TRingBuffer;
	static lockermap map_Clientlocker;
	static mutex_locker TRingBuffermaplocker; //互斥锁,锁缓冲区
	static mutex_locker clientlocker; //互斥锁，锁客户端类型
	static pNotifyFun notify_callback;
public:
	Task(char *str,int readlen, int fd) : sockfd(fd)
	{
		memset(order, 0, MAX_BUFFER);
		memcpy(order,str,readlen);
		orderlen = readlen;
	}
	void doit()  //任务的执行函数
	{
		//do something of the order
		processtask(order,orderlen,sockfd);
	}
	void processtask(char *buffer,int buflen,int fd);
	int ReadPacket(TRingBuffer *clientbuffer,char* szPacket, int iPackLen);
	static TRingBuffer* findtask(int fd);//通过fd查找环形队列指针，如果没有就new
	static mutex_locker* findclientlocker(TRingBuffer *pTRingBuffer);
	static void clearRingBuffer(int fd);
	static void updateclientmap(int fd,int type); //通过fd更新clienttypemap
	static int findclientfd(int type,int *fdarray); //通过客户端类型查找，返回查找到的fd个数，如果没有返回0
	static void clearclienttype(int fd);
	int dealprotocldata(const char* packet,int packetlen,int fd);
};
#endif