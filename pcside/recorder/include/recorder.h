#ifndef RECOREDER
#define RECOREDER
#include <atomic>
#include <list>
#include <ringbuffer.h>
#include <samples.h>
#include <vector>
namespace record {
enum states { RECORED, STOP };

class triger {
      public:
	virtual bool shouldtrigger(samples::sample);
	virtual ~triger();
	triger();
};

enum edgeTrigerType { RISEING, FALEING };
class edgetriger : public triger {
      private:
	samples::sample refv;
	samples::sample prev;
	enum edgeTrigerType type;

      public:
	bool shouldtrigger(samples::sample samp);
	edgetriger(samples::sample ref, enum edgeTrigerType type);
	~edgetriger();
};

class nevertriger : public triger {
      public:
	bool shouldtrigger(samples::sample samp);
	nevertriger();
	~nevertriger();
};
class recorder {
      private:
	triger *startTrig;
	triger *stopTrig;
	ringbuffers::ringbuffer *samplest;
	std::atomic<enum states> *recstate;
	pthread_t thId;
	helper::thwraper<std::vector<samples::sample>> buffer;
	unsigned int freq;
	static void executor(recorder *thispointer);

      public:
	/**
	 * this function takes ownship of the two trigers
	 */
	recorder(triger *startTriger, triger *stopTriger, ringbuffers::ringbuffer *samplestream,
		 std::atomic<enum states> *state, size_t buffsize, unsigned int freq = 40000);
	~recorder();
	std::vector<samples::sample> getRecords();
	std::vector<samples::sample> getRecords(unsigned int start, unsigned int stop);
	int buffersize();
	void clear();
};

class recorederstate {
      public:
	std::atomic<enum states> state;
	std::list<std::vector<recorder *>> recorders; // one vector for each device in the same order as devices
	recorederstate();
};
}; // namespace record
#endif
