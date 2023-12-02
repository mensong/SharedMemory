#ifndef __PROCEED_DATA_EXCHANGE_H   
#define __PROCEED_DATA_EXCHANGE_H

#include <windows.h>
#include <iostream>
#include <tchar.h>

class ProceedDataExchange
{
public:
	/// <summary>
	/// 小于0则表示错误
	/// </summary>
	enum RESULT
	{
		NEWDATA = 1,
		EXISTDATA = 0,

		NOEXISTDATA = -1,
		ASKFORWRITEFAIL = -2,
		ASKFORREADFAIL = -3,
	};

public:
	ProceedDataExchange(const TCHAR memoryName[], long int memorySize, BOOL bGlobal = FALSE);
	~ProceedDataExchange();

	bool isValid();

	//dataSize的读写长度需要一样才能成功
	RESULT writeData(const void* pData, long int dataSize, long int dataID, BOOL block);//write data
	RESULT readData(void* outData, long int dataSize, long int dataID, BOOL block);//read data

	//写数据，先写数据大小，紧接着才写数据
	RESULT writePackage(const void* pData, long int dataSize, long int dataID, BOOL block);//write data package
	//读数据，先读数据大小，紧接着才读数据
	RESULT readPackage(void* outData, long int& outDataSize, long int dataID, BOOL block);//read data package

	//原子性读写
	RESULT atomReadWriteData(void* pReadData, const void* pWriteData, long int dataSize, long int dataID, BOOL block);//atom read and write data

private:
	HANDLE hMapFile;				//map handle(data map)
	HANDLE hMapFileWRLock;			//(lock map)

	HANDLE hMutexWRLock;			//(mutex handle)
	HANDLE hEventWRLock;			//(event handle)

	HANDLE lockStart;				//Memory Start handle
	HANDLE hMutexWRLockStart;		//Lock Start handle

	LPCTSTR memoryPBuf;				//memory pointer
	LPCTSTR memoryPBufWRLock;		//lock memory pointer

	typedef struct
	{
		long int dataOffset[10];
		long int dataID[10];
		long int arrayLen;
	} DataT;
	DataT dataInform;

	int askWriteLock(BOOL);
	void unLockWriteLock(void);
	int askReadLock(BOOL);
	void unLockReadLock(void);

	RESULT rawWriteData(const void* pData, long int dataSize, long int dataID);//write data package
	RESULT rawReadData(void* pData, long int dataSize, long int dataID);//read data package

	void release();

};

#endif	
