/******************************************************************
* Copyright(c) 2020-2028 ZHENGZHOU Tiamaes LTD.
* All right reserved. See COPYRIGHT for detailed Information.
*
* @fileName: unixserver.cpp
* @brief: 域通信服务端实现
* @author: dinglf
* @date: 2020-05-06
* @history:
*******************************************************************/
#include "unixserver.h"
UnixServer* UnixServer::m_punixserver = NULL;
bool UnixServer::init(const char *socketfile,pNotifyFun callback)
{
	if(unlink(socketfile) < 0)	//解除原有对象链接
	{
		//printf("unlink socketfile failed.\n");
	}
	Task::notify_callback = callback;
	bzero(&ServerAddr, sizeof(ServerAddr));
	//配置ServerAddr
	ServerAddr.sun_family = AF_UNIX;
	strcpy(ServerAddr.sun_path,socketfile);
    //创建Socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        //printf("unixserver socket init error\n");
        return false;
    }

    int ret = bind(sockfd, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr));
    if(ret < 0)
    {
        //printf("unixserver bind init error\n");
        return false;
    }
    ret = listen(sockfd, 10);
    if(ret < 0)
    {
        //printf("unixserver listen init error\n");
        return false;
    }
    //create Epoll
    epollfd = epoll_create(1024);
    if(epollfd < 0)
    {
        //printf("unixserver epoll_create init error\n");
        return false;
    }
	//printf("unixserver init success.\n");
	return true;
    //pool = new threadpool<BaseTask>(threadnum);  //创建线程池	
}
bool UnixServer::startpool()
{
    pool = new threadpool<BaseTask>(threadnum);  //创建线程池
	if(NULL == pool)
	{
		//printf("pool=NULL.\n");
		return false;
	}
    pool->start();   //线程池启动
	return true;
}
void UnixServer::addclienttime(int fd) 
{
	unsigned long timestamp;
	timestamp = GetSysTime();
	activeconnectmaplocker.mutex_lock();
	m_activeclient.insert(pair<int,unsigned long>(fd,timestamp));
	m_connecttime.insert(pair<int,unsigned int>(fd,timestamp));
	activeconnectmaplocker.mutex_unlock();	
}
void UnixServer::removeclienttime(int fd) 
{
	activeconnectmaplocker.mutex_lock();
	map<int,unsigned long>::iterator iter;
	iter = m_activeclient.find(fd);
	if(iter != m_activeclient.end())
	{
		m_activeclient.erase(iter);
	}
	map<int,unsigned int>::iterator iter1;
	iter1 = m_connecttime.find(fd);
	if(iter1 != m_connecttime.end())
	{
		m_connecttime.erase(iter1);
	}
	activeconnectmaplocker.mutex_unlock();	
}
void UnixServer::clearclienttime()
{
	activeconnectmaplocker.mutex_lock();
	map<int,unsigned long>::iterator iter;
	for(iter=m_activeclient.begin();iter!=m_activeclient.end();)
	{
		m_activeclient.erase(iter++);
	}
	map<int,unsigned int>::iterator iter1;
	for(iter1=m_connecttime.begin();iter1!=m_connecttime.end();)
	{
		m_connecttime.erase(iter1++);
	}
	activeconnectmaplocker.mutex_unlock();		
}
void UnixServer::updateclienttime(int fd) 
{
	unsigned long timestamp;
	timestamp = GetSysTime();
	activeconnectmaplocker.mutex_lock();
	map<int,unsigned long>::iterator iter;
	iter = m_activeclient.find(fd);
	if(iter != m_activeclient.end())
	{
		m_activeclient[fd] = timestamp;
	}
	else{
		m_activeclient.insert(pair<int,unsigned long>(fd,timestamp));
	}
	activeconnectmaplocker.mutex_unlock();	
}
void UnixServer::epoll()
{
    //pool->start();   //线程池启动

    addfd(epollfd, sockfd, false);
    while(!is_stop)
    {//调用epoll_wait
        int ret = epoll_wait(epollfd, events, MAX_EVENT, -1);

        if(ret < 0)  //出错处理
        {
            //被信号中断
            if (errno == EINTR)
            {
                //printf("errno == EINTR\n");
                continue;
            }
            else
			{
                //printf("epoll_wait error\n");
                break;
            }
        }
        for(int i = 0; i < ret; ++i)
        {
            int fd = events[i].data.fd;
            if(fd == sockfd)  //新的连接到来
            {
				//printf("new client connect.\n");
                struct sockaddr_un clientAddr;
                socklen_t len = sizeof(clientAddr);
                int confd = accept(sockfd, (struct sockaddr *)
                        &clientAddr, &len);

                //printf("client confd: %d\n",confd);
				if(confd == -1)
				{
					//printf("errno = %d.\n",errno);
				}	
				addclienttime(confd);
                addfd(epollfd, confd, false);
            }
            else if(events[i].events & EPOLLIN)  //某个fd上有数据可读
            {
                char buffer[MAX_BUFFER];
                readagain:	memset(buffer, 0, sizeof(buffer));
                int ret = read(fd, buffer, MAX_BUFFER - 1);
                if(ret == 0)  //某个fd关闭了连接，从Epoll中删除并关闭fd
                {
                    struct epoll_event ev;
                    ev.events = EPOLLIN;
                    ev.data.fd = fd;
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
                    shutdown(fd, SHUT_RDWR);
					close(fd);
                    //printf("%d logout\n", fd);
					removeclienttime(fd);
					Task::clearRingBuffer(fd);
					Task::clearclienttype(fd);
                    continue;
                }
                else if(ret < 0)//读取出错，尝试再次读取
                {
                    if(errno == EAGAIN)
                    {
                        //printf("read error! errno = EAGAIN,continue.\n");
                        continue;
                    }
					else if(errno == EINTR)
					{
                        //printf("read error! errno = EINTR,readagain.\n");
                        goto readagain;						
					}
					else 
					{
						//printf("read error! errno = %d,break.\n",errno);
						break;
					}
                }
                else//成功读取，向线程池中添加任务
                {
                    //printf("received data,fd = %d,datalen = %d\n",fd,ret);
					BaseTask *task = NULL;
                    task = new Task(buffer,ret,fd);
					if(NULL == task)
					{
						//printf("task=NULL.\n");
						continue;
					}
					if(NULL == pool)
					{
						//printf("pool=NULL.\n");
						break;
					}
					////printf("epoll task = %p\n",task);
                    pool->append_task(task);
					updateclienttime(fd);
                }
            }
            else
            {
                //printf("something else had happened,fd = %d\n",fd);
            }
        }
    }
    close(sockfd);//结束。

    //pool->stop();
}
void UnixServer::disconnect(int fd)
{
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
    shutdown(fd, SHUT_RDWR);
    close(fd);
	removeclienttime(fd);
	Task::clearRingBuffer(fd);
	Task::clearclienttype(fd);
    //printf("%d logout\n", fd);
}
void UnixServer::stoppool()
{
	if(NULL == pool)
	{
		//printf("pool=NULL.\n");
		return;
	}
    pool->stop();   //线程池停止	
}