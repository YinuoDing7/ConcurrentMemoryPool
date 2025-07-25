#define  _CRT_SECURE_NO_WARNINGS 1
#include"CentralCache.h"
#include"PageCache.h"

CentralCache CentralCache::_sInst;

Span* CentralCache::GetOneSpan(SpanList& list, size_t size) {

	// �鿴��ǰSpanList���Ƿ���δ�����span
	Span* it = list.Begin();
	while (it != list.End()) {
		if (it->_freeList != nullptr) {
			return it;
		}
		else {
			it = it->_next;
		}
	}

	// ����˵��û�п��е�span�ˣ�ֻ����PageCacheҪ
	//�Ȱ�centralCache��Ͱ�������������������߳��ͷ��ڴ������������
	list._mtx.unlock();

	PageCache::GetInstance()->_pageMtx.lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	PageCache::GetInstance()->_pageMtx.unlock();


	//����Ҫ������ �Ի�ȡ����span�����з֣������̷߳��ʲ������span

	// ����span�Ĵ���ڴ����ʼ��ַ�ʹ���ڴ�Ĵ�С���ֽ�����
	//void* start = (void*)(span->_pageId << PAGE_SHIFT);
	//char* ����ָ���ƶ�����
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t bytes = span->_n << PAGE_SHIFT;
	char* end = start + bytes;

	// �Ѵ���ڴ��г�С�鲢�ҵ�����������
	// ����һ��������ͷ������β��
	span->_freeList = start;
	start += size;
	void* tail = span->_freeList;

	while (start < end) {
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += size;
	}

	//�к�span�Ժ� ��span�ҵ�Ͱ�ϣ�����
	list._mtx.lock();
	list.PushFront(span);

	return nullptr;
}

size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size) {

	size_t index = SizeClass::Index(size);
	_spanLists[index]._mtx.lock();

	Span* span = GetOneSpan(_spanLists[index], size);
	assert(span);
	assert(span->_freeList);

	start = span->_freeList;
	end = start;
	size_t actualNum = 1;
	for (int i = 0;i < batchNum - 1 && NextObj(end) != nullptr;i++) {//Ҳ�����˲���batchNum�����
		end = NextObj(end);
		++actualNum;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;

	_spanLists[index]._mtx.unlock();

	return actualNum;
}
