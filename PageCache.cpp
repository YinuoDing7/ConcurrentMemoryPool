#define  _CRT_SECURE_NO_WARNINGS 1
#include"PageCache.h"

PageCache PageCache::_sInst;

Span* PageCache::NewSpan(size_t k) {
	assert(k > 0 && k < NPAGES);

	// �ȼ���k��Ͱ����span
	if (!_spanLists[k].Empty()) {
		return _spanLists[k].PopFront();
	}

	// �������Ͱ����span��������з�span
	for (size_t i = k + 1;i < NPAGES;i++) {
		if (!_spanLists[i].Empty()) {
			Span* nSpan = _spanLists[i].PopFront();
			Span* kSpan = new Span;

			// ��nSpan��ͷ����һ��kҳ����
			// kҳ��span���أ�n-kҳspan�ҵ�n-k��Ͱ
			kSpan->_pageId = nSpan->_pageId;
			kSpan->_n = k;

			nSpan->_pageId += k;
			nSpan->_n -= k;

			_spanLists[i - k].PushFront(nSpan);

			return kSpan;
		}
	}

	//�ߵ���˵������û�д���kҳ��span��
	//�Ҷ�Ҫһ��128ҳ��span
	Span* bigSpan = new Span;
	void* ptr = SystemAlloc(NPAGES - 1);
	bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
	bigSpan->_n = NPAGES - 1;

	_spanLists[NPAGES - 1].PushFront(bigSpan);

	return NewSpan(k);//����һ���Լ� ��������߼�
}
