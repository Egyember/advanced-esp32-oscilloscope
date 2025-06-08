#ifndef SAMPLES
#define SAMPLES

#include <helpertypes.h>
#include <queue>
namespace samples{
	class sample{
		public:
		float voltage;
//		unsigned long long int time;
		sample(float volt);
		sample();
	};
	typedef helper::thwraper<std::deque<sample>> sampleStream;
}
#endif
