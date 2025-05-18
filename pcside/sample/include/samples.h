#ifndef SAMPLES
#define SAMPLES

#include <queue>
namespace samples{
	class sample{
		public:
		float voltage;
//		unsigned long long int time;
		sample(float volt);
	};
	typedef std::queue<sample> sampleStream;
}
#endif
