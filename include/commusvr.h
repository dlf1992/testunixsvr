/*
 * 	commusvr.h
 *
 */
 
/*条件编译*/
#ifndef COMMU_SVR_H_
#define COMMU_SVR_H_
//#include <iostream>
//#include <map>
//using namespace std;
 
#ifdef __cplusplus
extern "C"  //C++
{
#endif
typedef  int (*pNotifyFun)(int,int,int);//fd type cmd
int StartUnixService(const char* sockfile,pNotifyFun callback);
#ifdef __cplusplus
}
#endif
 
#endif /* COMMU_SVR_H_ */