// shm-test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <Windows.h>
#include <iostream>
#include <tchar.h>
#include "ProceedDataExchange.h"
#include <stdlib.h> 
#include <time.h> 

ProceedDataExchange shm(TEXT("memoryName"), 1024);

int main(int argc, char** argv)
{
	if (!shm.isValid())
		return 1;

	if (argc == 1)
	{
		srand(time(nullptr));

		char data[50];
		memset(data, 0, sizeof(data));
		while (true)
		{
			size_t randoxNumber = 1 + rand() % (100000 - 1);
			memcpy(data, &randoxNumber, sizeof(size_t));
			int res = shm.writeData(data, sizeof(size_t), 1, TRUE);
			std::cout << "write:" << randoxNumber << "\n";
		}
	}
	else
	{
		while (true)
		{
			size_t data = 0;
			int res = shm.readData(&data, sizeof(size_t), 1, TRUE);
			std::cout << "read:" << data << "\n";
		}
	}

    return 0;
}
