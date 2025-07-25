#pragma once
#include"Common.h"

class PageCache {
public:
	static PageCache* GetInstance() {
		return &_sInst;
	}

	// ��ȡһ��kҳ��span
	Span* NewSpan(size_t k);


private:
	SpanList _spanLists[NPAGES];
public:
	std::mutex _pageMtx;//һ�Ѵ���������Ͱ����

private:
	PageCache() {}
	PageCache(const PageCache&) = delete;

	static PageCache _sInst;//����ģʽ
};
