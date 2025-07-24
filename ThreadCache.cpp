#define  _CRT_SECURE_NO_WARNINGS 1
#include"ThreadCache.h"
#include"CentralCache.h"

void* ThreadCache::Allocate(size_t size) {
	assert(size <= MAX_BYTES);

	//����size
	size_t alignSize = SizeClass::RoundUp(size);
	//ӳ�䵽Ͱ�±�
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
	// ����ʼ���������㷨
	//1.�ʼ����һ����centralcache����Ҫ̫�࣬��Ϊ̫������ò���
	//2.����һֱҪ�����������maxsizeҲ��������
	//3.ֱ���ﵽ�Լ����趨������NumMoveSize

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
	else {//����һ����ʣ�µĹҵ�����������
		_freelists[index].PushRange(NextObj(start), end);
		return start;
	}
}