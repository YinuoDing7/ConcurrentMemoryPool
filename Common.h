#pragma once

#include<iostream>
#include<algorithm>
#include<assert.h>
#include<thread>
#include<mutex>
using std::cout;
using std::endl;

static const size_t MAX_BYTES = 256 * 1024;//小于256KB，找threadcache分配
static const size_t NFREELIST = 208;//threadcache/centralcache多少个桶/自由链表（MAX_BYTES和映射规则决定了的）

#ifdef _WIN64
	typedef unsigned long long PAGE_ID;
#elif _WIN32
	typedef size_t PAGE_ID;
#else 
	//Linux
#endif


static void*& NextObj(void* obj) {
	assert(obj);

	return *(void**)obj;
}

//管理切分好的小内存块的自由链表
class Freelist {
public:
	void Push(void* obj) {
		assert(obj);

		//头插
		//*(void**)obj = _freelist;//前4/8字节用来指向下一个内存块
		NextObj(obj) = _freelist;
		_freelist = obj;
	}
	void PushRange(void* start, void* end) {
		NextObj(end) = _freelist;
		_freelist = start;
	}
	void* Pop() {
		assert(_freelist);

		//头删
		void* obj = _freelist;
		_freelist = NextObj(_freelist);
		return obj;
	}
	//判空
	bool Empty() {
		return _freelist==nullptr;
	}
	size_t& MaxSize() {
		return _maxSize;
	}
private:
	void* _freelist=nullptr;
	size_t _maxSize = 1;
};


//内存大小与桶的映射规则
class SizeClass {
public:
	//整体控制在10%左右的内碎片浪费
	//[1,128]               8byte对齐    freelists[0,16)
	//[128+1,1024]          16byte对齐   freelists[16,72)
	//[1024+1,8*1024]       128byte对齐  freelists[72,128)
	// [8*1024+1,64*1024]   1024byte对齐 freelists[128,184)
	// [64*1024+1,256*1024] 8*1024对齐   freelists[184,208)   

	//计算内存对齐
	static inline size_t _RoundUp(size_t size, size_t alignNum) {//传内存大小和对齐数
		//其实就是找到与size最相近且大的alignNum的倍数
		
		//return (size+alignNum-1)&(alignNum-1);//位运算快

		size_t alignSize = 0;//对齐后的内存大小
		if (size % alignNum == 0) {
			alignSize = size;
		}
		else {
			alignSize = (size / alignNum + 1) * alignNum;
		}
		return alignSize;

	}
	static inline size_t RoundUp(size_t size) {
		
		if (size <= 128) {
			return _RoundUp(size, 8);
		}
		else if (size <= 1024) {
			return _RoundUp(size, 16);
		}
		else if (size <= 8 * 1024) {
			return _RoundUp(size, 128);
		}
		else if (size <= 64 * 1024) {
			return _RoundUp(size, 1024);
		}
		else if (size <= 256 * 1024) {
			return _RoundUp(size, 8 * 1024);
		}
		else {
			assert(false);
			return -1;
		}
	}

	//映射到几号桶
	static inline size_t _Index(size_t bytes, size_t align_shift) {//传对齐数是2的几次方（1左移几位）
		//这是相当于本区间的坐标

		return (bytes + (1 << align_shift - 1) >> align_shift) - 1;//右移那几位相当于整除了对齐数，这也是传位数的原因
	}
	static inline size_t Index(size_t bytes) {

		//每个区间的长度
		static int group_array[4] = { 16,56,56,56 };
		if (bytes <= 128) {
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024) {
			return _Index(bytes - 128, 4) + group_array[0];
		}
		else if (bytes <= 8 * 1024) {
			return _Index(bytes - 1024, 7) + group_array[0] + group_array[1];
		}
		else if (bytes <= 64 * 1024) {
			return _Index(bytes - 8 * 1024, 10) + group_array[0] + group_array[1] + group_array[2];
		}
		else if (bytes <= 256 * 1024) {
			return _Index(bytes - 64 * 1024, 13) + group_array[0] + group_array[1] + group_array[2] + group_array[3];
		}
		else {
			assert(false);
			return -1;
		}

	}

	//threadcache一次从centralcache获取内存对象个数的上限
	static size_t NumMoveSize(size_t size) {
		assert(size > 0);

		// 小对象一次批量上限高
		// 大对象一次批量上限低
		//【2，512】
		int num = MAX_BYTES / size;
		if (num < 2) num = 2;
		if (num > 512) num = 512;

		return num;
	}
};

// 管理多个连续页大块内存的跨度
struct Span {
	PAGE_ID _pageId = 0; // 大块内存起始页的页号
	size_t _n = 0;       // 含有多少页

	Span* _next = nullptr;     // 双向链表结构
	Span* _prev = nullptr;

	size_t _useCount = 0;// 分配给了几个ThreadCache
	void* _freeList = nullptr; // 切好的小块内存的自由链表
};

// 带头双向循环链表
class SpanList {
public:
	SpanList(){
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	void Insert(Span* pos, Span* newSpan) {
		assert(pos);
		assert(newSpan);

		Span* prev = pos->_prev;

		prev->_next = newSpan;
		newSpan->_prev = prev;
		newSpan->_next = pos;
		pos->_prev = newSpan;
	}
	void Erase(Span* pos) {
		assert(pos);
		assert(pos != _head);

		Span* prev = pos->_prev;
		Span* next = pos->_next;

		prev->_next = next;
		next->_prev = prev;
	}
private:
	Span* _head;
public:
	std::mutex _mtx; //加桶锁 
};