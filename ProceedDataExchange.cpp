#include "ProceedDataExchange.h"


ProceedDataExchange::ProceedDataExchange(const TCHAR memoryName[], long int memorySize, BOOL bGlobal/* = FALSE*/)
	: hMapFile(NULL)
	, hMapFileWRLock(NULL)
	, hMutexWRLock(NULL)
	, hEventWRLock(NULL)
	, lockStart(NULL)
	, hMutexWRLockStart(NULL)
	, memoryPBuf(NULL)
	, memoryPBufWRLock(NULL)
{
	memset(&dataInform, 0, sizeof(dataInform));

	TCHAR szMemoryName[MAX_PATH] = TEXT("");	
	if (bGlobal)
		lstrcat(szMemoryName, TEXT("Global\\"));
	else
		lstrcat(szMemoryName, TEXT("Local\\"));
	lstrcat(szMemoryName, memoryName);

	long int bufferSize = memorySize + 100;
	//CreateFileMapping
	hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // use paging file
		NULL,                    // default security
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD)
		bufferSize,             // maximum object size (low-order DWORD)
		szMemoryName);			// name of mapping object
	if (hMapFile == NULL)
	{
		DWORD e = GetLastError();
		printf("Create File Mapping Error:%d\n", e);
		return;
	}

	//MapViewOfFile
	memoryPBuf = (LPTSTR)MapViewOfFile(hMapFile,   // handle to map object
		FILE_MAP_ALL_ACCESS, // read/write permission
		0,
		0,
		bufferSize);
	if (memoryPBuf == NULL)
	{
		release();

		DWORD e = GetLastError();
		printf("Map View Of File Error:%d\n", e);
		return;
	}

	//Memory initial
	TCHAR intialLockName[MAX_PATH] = TEXT("");
	TCHAR lockinitial[] = TEXT("Mem_Init");
	lstrcat(intialLockName, szMemoryName);
	lstrcat(intialLockName, lockinitial);
	lockStart = CreateMutex(NULL, false, intialLockName);
	DWORD result = WaitForSingleObject(lockStart, 0);//…Í«ÎÀ¯
	if (WAIT_OBJECT_0 == result)
	{
		long int length = 0;
		memcpy((PVOID)memoryPBuf, &length, 4);
	}

	//Write and Read Lock initial-------------------------------------------------------------
	//=========================================================================================
	TCHAR WRLockName[MAX_PATH] = TEXT("");
	TCHAR WRLock[] = TEXT("WRLock");
	lstrcat(WRLockName, szMemoryName);
	lstrcat(WRLockName, WRLock);
	hMapFileWRLock = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // use paging file
		NULL,                    // default security
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD)
		2,						 // maximum object size (low-order DWORD)
		WRLockName);           // name of mapping object
	if (hMapFileWRLock == NULL)
	{
		release();

		DWORD e = GetLastError();
		printf("Create File Mapping WRLock error:%d\n", e);
		return;
	}
	memoryPBufWRLock = (LPTSTR)MapViewOfFile(hMapFileWRLock,   // handle to map object
		FILE_MAP_ALL_ACCESS, // read/write permission
		0,
		0,
		2);
	if (memoryPBufWRLock == NULL)
	{
		release();
		
		DWORD e = GetLastError();
		printf("Map View Of File WRLock error:%d\n", e);
		return;
	}

	//mutex init
	TCHAR mutexWRLockName[MAX_PATH] = TEXT("");
	TCHAR mutexWRLock[] = TEXT("mutexWRLock");
	lstrcat(mutexWRLockName, szMemoryName);
	lstrcat(mutexWRLockName, mutexWRLock);
	hMutexWRLock = CreateMutex(NULL, false, mutexWRLockName);
	if (NULL == hMutexWRLock)
	{
		release();

		DWORD e = GetLastError();
		printf("CreateMutex WRLock error:%d\n", e);
		return;
	}
	TCHAR mutexWRLockInitName[MAX_PATH] = TEXT("");
	TCHAR mutexWRLockInit[] = TEXT("mutexWRLockInit");
	lstrcat(mutexWRLockInitName, szMemoryName);
	lstrcat(mutexWRLockInitName, mutexWRLockInit);
	hMutexWRLockStart = CreateMutex(NULL, false, mutexWRLockInitName);
	if (NULL == hMutexWRLockStart)
	{
		release();

		DWORD e = GetLastError();
		printf("CreateMutex WRLock Init error:%d\n", e);
		return;
	}

	//event init
	TCHAR mutexWRLockEventName[MAX_PATH] = TEXT("");
	TCHAR mutexWRLockEvent[] = TEXT("mutexWRLockEvent");
	lstrcat(mutexWRLockEventName, szMemoryName);
	lstrcat(mutexWRLockEventName, mutexWRLockEvent);
	hEventWRLock = CreateEvent(NULL, TRUE, FALSE, mutexWRLockEventName);

	//memory init
	DWORD resultStart = WaitForSingleObject(hMutexWRLockStart, 0);//ask for lock
	if (WAIT_OBJECT_0 == resultStart)
	{
		int initial[2];
		initial[0] = 0;
		initial[1] = 0;
		memcpy((PVOID)memoryPBufWRLock, initial, sizeof(int) * 2);
	}
}

ProceedDataExchange::~ProceedDataExchange()
{
	release();
}

bool ProceedDataExchange::isValid()
{
	return hMapFile
		&& hMapFileWRLock
		&& hMutexWRLock
		&& hEventWRLock
		&& lockStart
		&& hMutexWRLockStart
		&& memoryPBuf
		&& memoryPBufWRLock;
}

int ProceedDataExchange::writePackage(const void* PData, long int dataSize, long int dataID, BOOL block)
{
	unsigned char newDataFlag = 1;
	long int dataOffsetIndex = 0;
	long int dataOffset = 0;
	//=============ask for write lock===================
	if (0 == askWriteLock(block))
	{
		return ASKFORWRITEFAIL;
	}
	//get data length
	memcpy(&dataInform.arrayLen, (PVOID)memoryPBuf, 4);
	//get data offset
	for (int i = 0; i < dataInform.arrayLen; i++)
	{
		memcpy(&dataInform.dataOffset[i], (PVOID)(memoryPBuf + 4 + 4 * i), 4);
	}
	//get data ID
	for (int i = 0; i < dataInform.arrayLen; i++)
	{
		memcpy(&dataInform.dataID[i], (PVOID)(memoryPBuf + 50 + 4 * i), 4);
		if (dataInform.dataID[i] == dataID && dataInform.dataOffset[i] == dataSize)
		{
			newDataFlag = 0;
			dataOffsetIndex = i;
		}
		if (newDataFlag == 1)
		{
			dataOffset += dataInform.dataOffset[i];
		}
	}
	if (newDataFlag == 1)
	{
		dataInform.dataID[dataInform.arrayLen] = dataID;
		dataInform.dataOffset[dataInform.arrayLen] = dataSize;
		dataInform.arrayLen++;
		memcpy((PVOID)(memoryPBuf + dataOffset + 100), PData, dataSize);
		memcpy((PVOID)memoryPBuf, &dataInform.arrayLen, 4);
		memcpy((PVOID)(memoryPBuf + 4 * dataInform.arrayLen), &dataInform.dataOffset[dataInform.arrayLen - 1], 4);
		memcpy((PVOID)(memoryPBuf + 50 + 4 * (dataInform.arrayLen - 1)), &dataInform.dataID[dataInform.arrayLen - 1], 4);

		unLockWriteLock();
		return NEWDATA;
	}
	else
	{
		memcpy((PVOID)(memoryPBuf + dataOffset + 100), PData, dataSize);

		unLockWriteLock();
		return EXISTDATA;
	}

}

int ProceedDataExchange::readPackage(void* PData, long int dataSize, long int dataID, BOOL block)
{
	unsigned char newDataFlag = 1;
	long int dataOffsetIndex = 0;
	long int dataOffset = 0;
	if (0 == askReadLock(block))
	{
		return ASKFORREADFAIL;
	}
	//get data length
	memcpy(&dataInform.arrayLen, (PVOID)memoryPBuf, 4);
	//get data offset
	for (int i = 0; i < dataInform.arrayLen; i++)
	{
		memcpy(&dataInform.dataOffset[i], (PVOID)(memoryPBuf + 4 + 4 * i), 4);
	}
	//get data ID
	for (int i = 0; i < dataInform.arrayLen; i++)
	{
		memcpy(&dataInform.dataID[i], (PVOID)(memoryPBuf + 50 + 4 * i), 4);
		if (dataInform.dataID[i] == dataID && dataInform.dataOffset[i] == dataSize)
		{
			newDataFlag = 0;
			dataOffsetIndex = i;
			break;
		}
		if (newDataFlag == 1)
		{
			dataOffset += dataInform.dataOffset[i];
		}
	}
	if (newDataFlag == 1)
	{
		unLockReadLock();
		return NOEXISTDATA;
	}
	else
	{
		memcpy(PData, (PVOID)(memoryPBuf + dataOffset + 100), dataSize);
		unLockReadLock();
		return EXISTDATA;
	}
}

int ProceedDataExchange::askWriteLock(BOOL block)
{
	int result = 0;
	while (1)
	{
		WaitForSingleObject(hMutexWRLock, INFINITE);//ask for lock
		int s[2];
		memcpy(s, (PVOID)memoryPBufWRLock, sizeof(int) * 2);
		//printf("%d %d\n", s[0], s[1]);
		if (s[0] == 0 && s[1] == 0)//s[0] write state	s[1] read state
		{
			s[0] = 1;
			memcpy((PVOID)memoryPBufWRLock, s, sizeof(int) * 2);
			ReleaseMutex(hMutexWRLock);
			result = 1;
			break;
		}
		else
		{
			ReleaseMutex(hMutexWRLock);
			if (block)
				WaitForSingleObject(hEventWRLock, INFINITE);
		}
	}
	return result;
}
void ProceedDataExchange::unLockWriteLock(void)
{
	WaitForSingleObject(hMutexWRLock, INFINITE);//ask for lock
	int s[2];
	memcpy(s, (PVOID)memoryPBufWRLock, sizeof(int) * 2);
	s[0] = 0;
	memcpy((PVOID)memoryPBufWRLock, s, sizeof(int) * 2);
	ReleaseMutex(hMutexWRLock);

	SetEvent(hEventWRLock);
	ResetEvent(hEventWRLock);
}
int ProceedDataExchange::askReadLock(BOOL block)
{
	int result = 0;
	while (1)
	{
		WaitForSingleObject(hMutexWRLock, INFINITE);//ask for lock
		int s[2];
		memcpy(s, (PVOID)memoryPBufWRLock, sizeof(int) * 2);
		if (s[0] == 0)
		{
			s[1]++;
			memcpy((PVOID)memoryPBufWRLock, s, sizeof(int) * 2);
			ReleaseMutex(hMutexWRLock);
			result = 1;
			break;
		}
		else
		{
			ReleaseMutex(hMutexWRLock);
			if (block)
				WaitForSingleObject(hEventWRLock, INFINITE);
		}
	}
	return result;
}
void ProceedDataExchange::unLockReadLock(void)
{
	WaitForSingleObject(hMutexWRLock, INFINITE);//ask for lock
	int s[2];
	memcpy(s, (PVOID)memoryPBufWRLock, sizeof(int) * 2);
	s[1]--;
	memcpy((PVOID)memoryPBufWRLock, s, sizeof(int) * 2);
	ReleaseMutex(hMutexWRLock);

	SetEvent(hEventWRLock);
	ResetEvent(hEventWRLock);
}

void ProceedDataExchange::release()
{
	if (hMapFile != NULL) 
	{
		CloseHandle(hMapFile);
		hMapFile = NULL;
	}
	if (hMapFileWRLock != NULL) 
	{ 
		CloseHandle(hMapFileWRLock); 
		hMapFileWRLock = NULL;
	}
	if (hMutexWRLock != NULL) 
	{
		CloseHandle(hMutexWRLock); 
		hMutexWRLock = NULL;
	}
	if (hEventWRLock != NULL) 
	{ 
		CloseHandle(hEventWRLock); 
		hEventWRLock = NULL;
	}
	if (lockStart != NULL)
	{
		CloseHandle(lockStart); 
		lockStart = NULL;
	}
	if (hMutexWRLockStart != NULL)
	{ 
		CloseHandle(hMutexWRLockStart); 
		hMutexWRLockStart = NULL;
	}
}
