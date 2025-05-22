#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <devices.h>

#define ANSIRED "\x1B[1;91m"
#define ANSIGREEN "\x1B[1;92m"
#define ANSIRESET "\x1B[0m"

int ring(){
	unsigned char tbuff[256] = {0};
	for (auto &c : tbuff) {
		c = std::rand();
	}

	unsigned char rbuff[256] = {0};

	ringbuffers::ringbuffer *ringb = new ringbuffers::ringbuffer(10*sizeof(tbuff));
	int ret = ringb->readBuffer(NULL, 0);
	if (ret != 0) {
		std::cout << "RINGBUFFER: " << ANSIRED << "empthy read returned wrong value (null dest)"<< "\n"<< ANSIRESET;
		return -1;
	}
	ret = ringb->readBuffer(rbuff, sizeof(rbuff));
	if (ret != 0) {
		std::cout << "RINGBUFFER: " << ANSIRED << "empthy read returned wrong value"<< "\n"<< ANSIRESET;
		return -1;
	}
	ringb->writeBuffer(tbuff, sizeof(tbuff));
	ringb->readBuffer(rbuff, sizeof(rbuff));
	ret = std::memcmp(tbuff, rbuff, sizeof(rbuff));
	if (ret != 0) {
		std::cout << "RINGBUFFER: " << ANSIRED << "simple write memory corruption memcmp() ret: "<< ret<<"\n"<< ANSIRESET;
		return -1;
		
	}
	delete ringb;
	return 0;
};

int pass;
int fail;
void ok(int err, std::string name){
	if (err != 0) {
		std::cout << ANSIRED << name << ": failed\n"<< ANSIRESET;
		fail++;
	}else {
		std::cout << ANSIGREEN << name << ": pass\n"<< ANSIRESET;
		pass++;
	}
};
int main(){
	std::srand(std::time({}));
	std::cout << "testing started\n";
	int err = ring();
	ok(err, "ringbuffer");

	if (fail > 0) {
		std::cout << ANSIRED << "tests failed\n"<< ANSIRESET;
	}
}
