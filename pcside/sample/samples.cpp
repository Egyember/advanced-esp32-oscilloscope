#include "./include/samples.h"
using namespace samples;

sample::sample(float volt){
	this->voltage = volt;
	this->chan = -1;
};

sample::sample(float volt, uint16_t chan){
	this->voltage = volt;
	this->chan = chan;
};

sample::sample(){
};
