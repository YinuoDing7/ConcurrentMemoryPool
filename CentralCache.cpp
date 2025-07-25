#define  _CRT_SECURE_NO_WARNINGS 1
#include"CentralCache.h"
#include"PageCache.h"

CentralCache CentralCache::_sInst;

Span* CentralCache::GetOneSpan(SpanList& list, size_t size) {

	// 查看当前SpanList中是否还有未分配的span
	Span* it = list.Begin();
	while (it != list.End()) {
		if (it->_freeList != nullptr) {
			return it;
		}
		else {
			it = it->_next;
		}
	}

	// 到这说明没有空闲的span了，只能找PageCache要
	//先把centralCache的桶锁解掉，这样如果其他线程释放内存回来不会阻塞
	list._mtx.unlock();

	PageCache::GetInstance()->_pageMtx.lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	PageCache::GetInstance()->_pageMtx.unlock();


	//不需要加锁了 对获取到的span进行切分，其他线程访问不到这个span

	// 计算span的大块内存的起始地址和大块内存的大小（字节数）
	//void* start = (void*)(span->_pageId << PAGE_SHIFT);
	//char* 方便指针移动来切
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t bytes = span->_n << PAGE_SHIFT;
	char* end = start + bytes;

	// 把大块内存切成小块并挂到自由链表上
	// 先切一块下来做头、方便尾插
	span->_freeList = start;
	start += size;
	void* tail = span->_freeList;

	while (start < end) {
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += size;
	}

	//切好span以后 把span挂到桶上，加锁
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
	for (int i = 0;i < batchNum - 1 && NextObj(end) != nullptr;i++) {//也包含了不够batchNum的情况
		end = NextObj(end);
		++actualNum;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;

	_spanLists[index]._mtx.unlock();

	return actualNum;
}
