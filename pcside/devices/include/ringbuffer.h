#ifndef RINGBUFFER
#define RINGBUFFER
#include <pthread.h>
#include <vector>
#include <atomic>
namespace ringbuffers {

	class ringbuffer{
		private:
			std::vector<unsigned char> *_data;
			bool empty;
			pthread_mutex_t lock;
			size_t readindex;
			size_t writeindex;
			size_t maxindex;
		public:
			//int initBuffer(size_t size);
			ringbuffer(size_t size);
			~ringbuffer();
			int readBuffer(unsigned char *dest, size_t size);
			size_t writeBuffer(unsigned char *dest, size_t size);
#ifdef DEBUGRINGBUFFER
			int print();
			int print(int hexwith);
#endif

	};
}

#endif
