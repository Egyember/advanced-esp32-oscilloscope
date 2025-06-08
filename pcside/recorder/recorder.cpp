#include <pthread.h>
#include <recorder.h>
#include <samples.h>
#include <unistd.h>
#include <vector>

using namespace record;

triger::~triger(){};

triger::triger(){};

bool triger::shouldtrigger(samples::sample){
	return false;
};
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

recorder::recorder(triger *startTriger, triger *stopTriger, samples::sampleStream *samplestream,
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
	while(true) {
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		thispointer->samplest->rdlock();
		bool emp = thispointer->samplest->_data.empty();
		thispointer->samplest->unlock();
		if(!emp) {
			thispointer->samplest->wrlock();
			samples::sample sampl = thispointer->samplest->_data.front();
			thispointer->samplest->_data.pop_front();
			thispointer->samplest->unlock();
			if(*thispointer->recstate == RECORED) {
				thispointer->buffer.wrlock();
				thispointer->buffer._data.push_back(sampl);
				thispointer->buffer.unlock();
				if(thispointer->stopTrig->shouldtrigger(sampl)) {
					*thispointer->recstate = STOP;
				};
			} else {
				if(thispointer->startTrig->shouldtrigger(sampl)) {
					*thispointer->recstate = RECORED;
				};
			}
		}else {
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


recorederstate::recorederstate(){
	state = STOP;
};
