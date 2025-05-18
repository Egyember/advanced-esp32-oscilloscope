#ifndef espSideTypes
#define espSideTypes
#include <stdint.h>
#include <stddef.h>
namespace esp{
struct  scopeConf {
	uint8_t channels;
	uint32_t sampleRate; // expected ADC sampling frequency in Hz.
	uint32_t duration;   // in ms
};
	static const size_t SOC_ADC_DIGI_RESULT_BYTES = (size_t)2;
}
#endif
