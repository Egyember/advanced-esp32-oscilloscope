#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <devices.h>
#include <helpertypes.h>

#define ANSIRED "\x1B[1;91m"
#define ANSIGREEN "\x1B[1;92m"
#define ANSIRESET "\x1B[0m"


int ring(){
	unsigned char tbuff[100] = {0};
	size_t halfsize = sizeof(tbuff)/2;
	size_t fullsize = sizeof(tbuff);
	for (size_t i = 0; i< fullsize; i++) {
		tbuff[i] = std::rand();
	}

	unsigned char rbuff[100] = {0};

	ringbuffers::ringbuffer *ringb = new ringbuffers::ringbuffer(10*fullsize);
	size_t ret = ringb->readBuffer(NULL, 0);
	if (ret != 0) {
		std::cout << "RINGBUFFER: " << ANSIRED << "empthy read returned wrong value (null dest)"  << "at: " << __LINE__<< "\n"<< ANSIRESET;
		delete ringb;
		return -1;
	}
	ret = ringb->readBuffer(rbuff, fullsize);
	if (ret != 0) {
		std::cout << "RINGBUFFER: " << ANSIRED << "empthy read returned wrong value"  << "at: " << __LINE__<< "\n"<< ANSIRESET;
		delete ringb;
		return -1;
	}
	ringb->writeBuffer(tbuff, fullsize);
	ringb->readBuffer(rbuff, fullsize);
	ret = std::memcmp(tbuff, rbuff, fullsize);
	if (ret != 0) {
		std::cout << "RINGBUFFER: " << ANSIRED << "simple write memory corruption memcmp() ret: "<< ret  << "at: " << __LINE__<<"\n"<< ANSIRESET;
		delete ringb;
		return -1;

	}
	size_t r;
	for (int i = 0 ; i<8;i++ ) {
		r = ringb->writeBuffer(tbuff, fullsize);
		if (r !=fullsize) {
			std::cout << "RINGBUFFER: " << ANSIRED << "write returned wrong size: "<< r   << "at: " << __LINE__<<"\n"<< ANSIRESET;
			return -1;
		}
	}

	for (int i = 0 ; i<8;i++ ) {
		r = ringb->readBuffer(rbuff, fullsize); 
		if (r !=fullsize) {
			std::cout << "RINGBUFFER: " << ANSIRED << "read returned wrong size: "<< r   << "at: " << __LINE__<<"\n"<< ANSIRESET;
			return -1;
		}
	}
	r = ringb->writeBuffer(tbuff, halfsize);
	if (r !=halfsize) {
		std::cout << "RINGBUFFER: " << ANSIRED << "write returned wrong size: "<< r   << "at: " << __LINE__<<"\n"<< ANSIRESET;
		return -1;
	}
	r = ringb->readBuffer(rbuff, halfsize); 
	if (r !=halfsize) {
		std::cout << "RINGBUFFER: " << ANSIRED << "read returned wrong size: "<< r   << "at: " << __LINE__<<"\n"<< ANSIRESET;
		return -1;
	}
	r = ringb->writeBuffer(tbuff, fullsize);
	if (r !=fullsize) {
		std::cout << "RINGBUFFER: " << ANSIRED << "write returned wrong size: "<< r   << "at: " << __LINE__<<"\n"<< ANSIRESET;
		return -1;
	}
	r = ringb->readBuffer(rbuff, fullsize); 
	if (r !=fullsize) {
		std::cout << "RINGBUFFER: " << ANSIRED << "read returned wrong size: "<< r   << "at: " << __LINE__<<"\n"<< ANSIRESET;
		return -1;
	}

	ret = std::memcmp(tbuff, rbuff, fullsize);
	if (ret != 0) {
		std::cout << "RINGBUFFER: " << ANSIRED << "write memory corruption (at overroll) memcmp() ret: "<< ret  << "at: " << __LINE__<<"\n"<< ANSIRESET;
		std::cout << "tbuff:\n";
		helper::hexdump(tbuff, sizeof(tbuff), 10);
		std::cout << "rbuff:\n";
		helper::hexdump(rbuff, sizeof(rbuff), 10);
		delete ringb;
		return -1;

	}
	r = ringb->writeBuffer(tbuff, fullsize);
	if (r !=fullsize) {
		std::cout << "RINGBUFFER: " << ANSIRED << "write returned wrong size: "<< r  << "at: " << __LINE__ <<"\n"<< ANSIRESET;
		return -1;
	}
	r = ringb->readBuffer(rbuff, fullsize); 
	if (r !=fullsize) {
		std::cout << "RINGBUFFER: " << ANSIRED << "read returned wrong size: "<< r << "at: " << __LINE__ <<"\n"<< ANSIRESET;
		return -1;
	}
	ret = std::memcmp(tbuff, rbuff, fullsize);
	if (ret != 0) {
		std::cout << "RINGBUFFER: " << ANSIRED << "write memory corruption (after overroll) memcmp() ret: "<< ret  << "at: " << __LINE__<<"\n"<< ANSIRESET;
		std::cout << "tbuff:\n";
		helper::hexdump(tbuff, sizeof(tbuff), 10);
		std::cout << "rbuff:\n";
		helper::hexdump(rbuff, sizeof(rbuff), 10);
		delete ringb;
		return -1;

	}
	r = ringb->writeBuffer(tbuff, halfsize);
	if (r !=halfsize) {
		std::cout << "RINGBUFFER: " << ANSIRED << "write returned wrong size: "<< r  << "at: " << __LINE__ <<"\n"<< ANSIRESET;
		return -1;
	}
	r = ringb->readBuffer(rbuff, fullsize); 
	if (r !=halfsize) {
		std::cout << "RINGBUFFER: " << ANSIRED << "read returned wrong size: "<< r << "at: " << __LINE__ <<"\n"<< ANSIRESET;
		return -1;
	}
	r = ringb->readBuffer(rbuff, fullsize); 
	if (r !=0) {
		std::cout << "RINGBUFFER: " << ANSIRED << "read returned wrong size: "<< r << "at: " << __LINE__ <<"\n"<< ANSIRESET;
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
	}else {
		std::cout << ANSIGREEN << "tests passed\n"<< ANSIRESET;
	}
}
