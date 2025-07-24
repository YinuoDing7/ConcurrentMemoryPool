#define  _CRT_SECURE_NO_WARNINGS 1
#include"ConcurrentAlloc.h"

void Alloc1() {
	for (size_t i = 0;i < 5;i++) {
		void* ptr = ConcurrentAlloc(6);
		//cout << "t1" << ptr << endl;
	}
}
void Alloc2() {
	for (size_t i = 0;i < 5;i++) {
		void* ptr = ConcurrentAlloc(7);
		//cout << "t2" << ptr << endl;
	}
}

void TLSTest() {
	std::thread t1(Alloc1);
	std::thread t2(Alloc2);

	t1.join();
	t2.join();
}

int main() {

	TLSTest();
	return 0;
}