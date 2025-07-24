#define  _CRT_SECURE_NO_WARNINGS 1
#include"ThreadCache.h"
#include"CentralCache.h"

void* ThreadCache::Allocate(size_t size) {
	assert(size <= MAX_BYTES);

	//对齐size
	size_t alignSize = SizeClass::RoundUp(size);
	//映射到桶下标
	size_t index = SizeClass::Index(size);

	if (!_freelists[index].Empty()) {
		return _freelists[index].Pop();
	}
	else {
		return FetchFromCentralCache(index, alignSize);
	}
}

void ThreadCache::Deallocate(void* ptr, size_t size) {
	assert(ptr);
	assert(size <= MAX_BYTES);
	
	size_t index = SizeClass::Index(size);
	_freelists[index].Push(ptr);
}

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size) {
	// 慢开始反馈调节算法
	//1.最开始不会一次向centralcache批量要太多，因为太多可能用不完
	//2.随着一直要，自由链表的maxsize也慢慢增大
	//3.直到达到自己被设定的上限NumMoveSize

	size_t batchNum = std::min(_freelists[index].MaxSize(),SizeClass::NumMoveSize(size));
	if (_freelists[index].MaxSize() == batchNum) {
		_freelists[index].MaxSize() += 1;
	}
	void* start = nullptr;
	void* end = nullptr;
	size_t actualNum=CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);
	assert(actualNum >= 1);

	if (actualNum == 1) {
		return start;
	}
	else {//返回一个，剩下的挂到自由链表上
		_freelists[index].PushRange(NextObj(start), end);
		return start;
	}
}