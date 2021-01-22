#include <stdio.h>
#include <stdlib.h>
#include "unixserver.h"
#include "commusvr.h"
#include "taskprocess.h"

int StartUnixService(const char* sockfile,pNotifyFun callback)
{
	UnixServer * punixserver = UnixServer::GetInstance();
	if(NULL == punixserver)
	{
		//printf("punixserver == NULL.\n");
		return -1;
	}
	if(punixserver->init(sockfile,callback))
	{
		if(punixserver->startpool())
		{
			punixserver->epoll();	
		}	
	}
	else
	{
		//printf("unixserver init failed.\n");
		UnixServer::Destroy();
		return -1;		
	}
	return 0;
}