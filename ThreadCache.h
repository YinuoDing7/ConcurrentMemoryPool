#pragma once
#include"Common.h"

class ThreadCache {
public:
	//申请、释放内存对象
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size); 

	//从CentralCache获取内存对象
	void* FetchFromCentralCache(size_t index, size_t size);
private:
	Freelist _freelists[NFREELIST];
};

//TLS
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;