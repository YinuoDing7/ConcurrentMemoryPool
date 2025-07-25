#pragma once

#include"Common.h"

class CentralCache {
public:
	static CentralCache* GetInstance() {
		return &_sInst;
	}

	// 找PageCache获取一个非空的span
	Span* GetOneSpan(SpanList& list, size_t size);

	// 从CentralCache获取一定数量的对象给ThreadCache
	size_t FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size);//输出型参数
private:
	SpanList _spanLists[NFREELIST];

private:
	CentralCache() {}

	CentralCache(const CentralCache&) = delete;
	static CentralCache _sInst;//单例模式(饿汉)
};
