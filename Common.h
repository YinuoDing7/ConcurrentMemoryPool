#pragma once

#include<iostream>
#include<algorithm>
#include<assert.h>
#include<thread>
#include<mutex>
using std::cout;
using std::endl;

static const size_t MAX_BYTES = 256 * 1024;//С��256KB����threadcache����
static const size_t NFREELIST = 208;//threadcache/centralcache���ٸ�Ͱ/��������MAX_BYTES��ӳ���������˵ģ�

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

//�����зֺõ�С�ڴ�����������
class Freelist {
public:
	void Push(void* obj) {
		assert(obj);

		//ͷ��
		//*(void**)obj = _freelist;//ǰ4/8�ֽ�����ָ����һ���ڴ��
		NextObj(obj) = _freelist;
		_freelist = obj;
	}
	void PushRange(void* start, void* end) {
		NextObj(end) = _freelist;
		_freelist = start;
	}
	void* Pop() {
		assert(_freelist);

		//ͷɾ
		void* obj = _freelist;
		_freelist = NextObj(_freelist);
		return obj;
	}
	//�п�
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


//�ڴ��С��Ͱ��ӳ�����
class SizeClass {
public:
	//���������10%���ҵ�����Ƭ�˷�
	//[1,128]               8byte����    freelists[0,16)
	//[128+1,1024]          16byte����   freelists[16,72)
	//[1024+1,8*1024]       128byte����  freelists[72,128)
	// [8*1024+1,64*1024]   1024byte���� freelists[128,184)
	// [64*1024+1,256*1024] 8*1024����   freelists[184,208)   

	//�����ڴ����
	static inline size_t _RoundUp(size_t size, size_t alignNum) {//���ڴ��С�Ͷ�����
		//��ʵ�����ҵ���size������Ҵ��alignNum�ı���
		
		//return (size+alignNum-1)&(alignNum-1);//λ�����

		size_t alignSize = 0;//�������ڴ��С
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

	//ӳ�䵽����Ͱ
	static inline size_t _Index(size_t bytes, size_t align_shift) {//����������2�ļ��η���1���Ƽ�λ��
		//�����൱�ڱ����������

		return (bytes + (1 << align_shift - 1) >> align_shift) - 1;//�����Ǽ�λ�൱�������˶���������Ҳ�Ǵ�λ����ԭ��
	}
	static inline size_t Index(size_t bytes) {

		//ÿ������ĳ���
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

	//threadcacheһ�δ�centralcache��ȡ�ڴ�������������
	static size_t NumMoveSize(size_t size) {
		assert(size > 0);

		// С����һ���������޸�
		// �����һ���������޵�
		//��2��512��
		int num = MAX_BYTES / size;
		if (num < 2) num = 2;
		if (num > 512) num = 512;

		return num;
	}
};

// ����������ҳ����ڴ�Ŀ��
struct Span {
	PAGE_ID _pageId = 0; // ����ڴ���ʼҳ��ҳ��
	size_t _n = 0;       // ���ж���ҳ

	Span* _next = nullptr;     // ˫������ṹ
	Span* _prev = nullptr;

	size_t _useCount = 0;// ������˼���ThreadCache
	void* _freeList = nullptr; // �кõ�С���ڴ����������
};

// ��ͷ˫��ѭ������
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
	std::mutex _mtx; //��Ͱ�� 
};