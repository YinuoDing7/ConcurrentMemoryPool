#pragma once

#include"Common.h"

class CentralCache {
public:
	static CentralCache* GetInstance() {
		return &_sInst;
	}

	// ��PageCache��ȡһ���ǿյ�span
	Span* GetOneSpan(SpanList& list, size_t size);

	// ��CentralCache��ȡһ�������Ķ����ThreadCache
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);//����Ͳ���
private:
	SpanList _spanLists[NFREELIST];

private:
	CentralCache() {}

	CentralCache(const CentralCache&) = delete;
	static CentralCache _sInst;//����ģʽ(����)
};
