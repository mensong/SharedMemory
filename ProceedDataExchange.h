#ifndef __PROCEED_DATA_EXCHANGE_H   
#define __PROCEED_DATA_EXCHANGE_H

#include <windows.h>
#include <iostream>
#include <tchar.h>

#define NEWDATA 1
#define EXISTDATA 0
#define NOEXISTDATA -1

#define ASKFORWRITEFAIL -2
#define ASKFORREADFAIL -3

using namespace std;

typedef struct
{
	long int dataOffset[10];
	long int dataID[10];
	long int arrayLen;
} DataT;

class ProceedDataExchange
{
public:
	ProceedDataExchange(const TCHAR memoryName[], long int memorySize, BOOL bGlobal = FALSE);
	~ProceedDataExchange();

	bool isValid();

	int writePackage(const void* PData, long int dataSize, long int dataID, BOOL block);//write data package
	int readPackage(void* PData, long int dataSize, long int dataID, BOOL block);//read data package

private:
	HANDLE hMapFile;				//map handle(data map)
	HANDLE hMapFileWRLock;			//(lock map)

	HANDLE hMutexWRLock;			//(mutex handle)
	HANDLE hEventWRLock;			//(event handle)

	HANDLE lockStart;				//Memory Start handle
	HANDLE hMutexWRLockStart;		//Lock Start handle

	LPCTSTR memoryPBuf;				//memory pointer
	LPCTSTR memoryPBufWRLock;		//lock memory pointer
	DataT dataInform;

	int askWriteLock(BOOL);
	void unLockWriteLock(void);
	int askReadLock(BOOL);
	void unLockReadLock(void);

	void release();

};

#endif	
