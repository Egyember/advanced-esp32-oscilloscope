#ifndef SAMPLES
#define SAMPLES

#include <helpertypes.h>
#include <deque>
#include <stdint.h>
namespace samples{
	class sample{
		public:
		float voltage;
		uint16_t chan;
//		unsigned long long int time;
		sample(float volt);
		sample(float volt, uint16_t chan);
		sample();
	};
}
#endif
