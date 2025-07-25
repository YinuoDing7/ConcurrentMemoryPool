#define  _CRT_SECURE_NO_WARNINGS 1
#include"PageCache.h"

PageCache PageCache::_sInst;

Span* PageCache::NewSpan(size_t k) {
	assert(k > 0 && k < NPAGES);

	// 先检查第k个桶有无span
	if (!_spanLists[k].Empty()) {
		return _spanLists[k].PopFront();
	}

	// 检查后面的桶有无span如果有则切分span
	for (size_t i = k + 1;i < NPAGES;i++) {
		if (!_spanLists[i].Empty()) {
			Span* nSpan = _spanLists[i].PopFront();
			Span* kSpan = new Span;

			// 在nSpan的头部切一个k页下来
			// k页的span返回；n-k页span挂到n-k号桶
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;

			nSpan->_pageId += k;
			nSpan->_n -= k;

			_spanLists[i - k].PushFront(nSpan);

			return kSpan;
		}
	}

	//走到这说明后面没有大于k页的span了
	//找堆要一个128页的span
	Span* bigSpan = new Span;
	void* ptr = SystemAlloc(NPAGES - 1);
	bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
	bigSpan->_n = NPAGES - 1;

	_spanLists[NPAGES - 1].PushFront(bigSpan);

	return NewSpan(k);//调用一下自己 走上面的逻辑
}
