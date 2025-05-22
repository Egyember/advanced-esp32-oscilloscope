#include <cstdlib>
#include <iostream>
#include <devices.h>

#define ANSIRED "\x1B[1;91m"
#define ANSIGREEN "\x1B[1;92m"
#define ANSIRESET "\x1B[0m"

int ring(){
	ringbuffers::ringbuffer *ringb = new ringbuffers::ringbuffer(3200);
	int rd = ringb->readBuffer(NULL, 0);
	if (rd != 0) {
		std::cout << "RINGBUFFER: " << ANSIRED << "empthy read returned wrong value"<< "\n"<< ANSIRESET;
		return -1;
	}
	unsigned char tbuff[256] = {0};
	for (auto &c : tbuff) {
		c = std::rand();
	}
	ringb->writeBuffer(tbuff, sizeof(tbuff));
	unsigned char rbuff[256] = {0};
	ringb->readBuffer(rbuff, sizeof(rbuff));

	delete ringb;
	return 0;
};
void ok(int err){
	if (err != 0) {
		std::cout << ANSIRED << "testing failed"<< "\n"<< ANSIRESET;
	}
};
int main(){
	std::srand(std::time({}));
	std::cout << "testing started\n";
	int err = ring();
	ok(err);
}
