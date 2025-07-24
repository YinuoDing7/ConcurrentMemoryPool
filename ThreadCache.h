#pragma once
#include"Common.h"

class ThreadCache {
public:
	//���롢�ͷ��ڴ����
	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size); 

	//��CentralCache��ȡ�ڴ����
	void* FetchFromCentralCache(size_t index, size_t size);
private:
	Freelist _freelists[NFREELIST];
};

//TLS
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;