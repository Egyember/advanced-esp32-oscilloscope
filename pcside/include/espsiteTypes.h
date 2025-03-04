#ifndef espSideTypes
#define espSideTypes
#include <stdint.h>
struct  scopeConf {
	uint8_t channels;
	uint32_t sampleRate; // expected ADC sampling frequency in Hz.
	uint32_t duration;   // in ms
};
#endif
