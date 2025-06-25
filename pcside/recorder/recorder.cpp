#include "devices.h"
#include <cstdio>
#include <ctime>
#include <pthread.h>
#include <recorder.h>
#include <ringbuffer.h>
#include <samples.h>
#include <unistd.h>
#include <vector>
#include <chrono>
#include <unistd.h>

#define READBUFFERSIZE 128

using namespace record;

triger::~triger() {};

triger::triger() {};

bool triger::shouldtrigger(samples::sample) { return false; };
edgetriger::edgetriger(samples::sample ref, enum edgeTrigerType type) {
	this->refv = ref;
	this->type = type;
	this->prev = 0;
};
bool edgetriger::shouldtrigger(samples::sample samp) {
	if(type == RISEING) {
		if(samp.voltage > refv.voltage) {
			if(samp.voltage > prev.voltage) {
				prev = samp;
				return true;
			}
			prev = samp;
		}
		return false;
	} else {
		if(samp.voltage < refv.voltage) {
			if(samp.voltage < prev.voltage) {
				prev = samp;
				return true;
			}
			prev = samp;
		}
		return false;
	}
};
edgetriger::~edgetriger() { return; };

bool nevertriger::shouldtrigger(samples::sample samp) { return false; };
nevertriger::nevertriger() {};
nevertriger::~nevertriger() { return; };

recorder::recorder(triger *startTriger, triger *stopTriger, ringbuffers::ringbuffer *samplestream,
		   std::atomic<enum states> *state, size_t buffsize, unsigned int freqarg) {
	freq = freqarg;
	startTrig = startTriger;
	stopTrig = stopTriger;
	samplest = samplestream;
	recstate = state;
	buffer.wrlock();
	buffer._data.reserve(buffsize);
	buffer.unlock();
	pthread_create(&thId, NULL, (void *(*)(void *))this->executor, this);
};

recorder::~recorder() {
	pthread_cancel(thId);
	delete this->startTrig;
	delete this->stopTrig;
};

void recorder::executor(recorder *thispointer) {
	uint16_t readbuff[READBUFFERSIZE] = {0};
	size_t read = 0;
	while(true) {
	//	std::chrono::microseconds start = std::chrono::duration_cast< std::chrono::microseconds >( std::chrono::system_clock::now().time_since_epoch());
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		read = thispointer->samplest->readBuffer((unsigned char *)readbuff, READBUFFERSIZE*sizeof(uint16_t));
	//	period *= read == 0? 10 : read;
		if(read != 0) {
			for (int i = 0; i < read/sizeof(uint16_t); i++) {
				auto samp = devices::parseSample(readbuff[i]);
				switch (*thispointer->recstate) {
					case RECORED:
						thispointer->buffer.wrlock();
						thispointer->buffer._data.push_back(samp);
						thispointer->buffer.unlock();
						if(thispointer->stopTrig->shouldtrigger(samp)) {
							*thispointer->recstate = STOP;
						};
						break;
					case STOP:
						if(thispointer->startTrig->shouldtrigger(samp)) {
							*thispointer->recstate = RECORED;
						};
						break;
				};

			}
		}else{

			long int period = (1000000 / thispointer->freq);
#ifdef NDEBUG
			printf("sleep %ld\n", period);
#endif
			usleep(period);
		}
	}
};

std::vector<samples::sample> recorder::getRecords() {
	this->buffer.rdlock();
	std::vector<samples::sample> ret(this->buffer._data.begin(), this->buffer._data.end());
	this->buffer.unlock();
	return ret;
};

std::vector<samples::sample> recorder::getRecords(unsigned int start,unsigned int stop) {
	this->buffer.rdlock();
	std::vector<samples::sample> ret(this->buffer._data.begin()+start, this->buffer._data.begin()+stop);
	this->buffer.unlock();
	return ret;
};

int recorder::buffersize(){
	this->buffer.rdlock();
	auto ret = this->buffer._data.size();
	this->buffer.unlock();
	return ret;

}
void recorder::clear() {
	this->buffer.wrlock();
	this->buffer._data.clear();
	this->buffer.unlock();
	this->samplest->clear();
};

recorederstate::recorederstate() { state = STOP; };
