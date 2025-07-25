#pragma once
#include"Common.h"

class PageCache {
public:
	static PageCache* GetInstance() {
		return &_sInst;
	}

	// 获取一个k页的span
	Span* NewSpan(size_t k);


private:
	SpanList _spanLists[NPAGES];
public:
	std::mutex _pageMtx;//一把大锁，不是桶锁了

private:
	PageCache() {}
	PageCache(const PageCache&) = delete;

	static PageCache _sInst;//单例模式
};
