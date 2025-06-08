#include "devices.h"
#include <pthread.h>
#include <recorder.h>
#include <ringbuffer.h>
#include <samples.h>
#include <unistd.h>
#include <vector>

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
		   std::atomic<enum states> *state, size_t buffsize) {
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
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		read = thispointer->samplest->readBuffer((unsigned char *)readbuff, READBUFFERSIZE*sizeof(uint16_t));
		if(read != 0) {
			for (int i = 0; i < read/sizeof(uint16_t); i++) {
				auto samp = devices::parseSample(readbuff[i]);
				if(*thispointer->recstate == RECORED) {
					thispointer->buffer.wrlock();
					thispointer->buffer._data.push_back(samp);
					thispointer->buffer.unlock();
					if(thispointer->stopTrig->shouldtrigger(samp)) {
						*thispointer->recstate = STOP;
					};
				} else {
					if(thispointer->startTrig->shouldtrigger(samp)) {
						*thispointer->recstate = RECORED;
					};
				}

			}
		} else {
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
			usleep(50);
		}
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	}
};

std::vector<samples::sample> recorder::getRecords() {
	this->buffer.rdlock();
	std::vector<samples::sample> ret(this->buffer._data.begin(), this->buffer._data.end());
	this->buffer.unlock();
	return ret;
};

void recorder::clear() {
	this->buffer.wrlock();
	this->buffer._data.clear();
	this->buffer.unlock();
};

recorederstate::recorederstate() { state = STOP; };
