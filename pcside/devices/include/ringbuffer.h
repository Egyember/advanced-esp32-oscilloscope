#ifndef RINGBUFFER
#define RINGBUFFER
#include <pthread.h>
#include <vector>
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
			size_t readBuffer(unsigned char *dest, size_t size);
			size_t writeBuffer(unsigned char *dest, size_t size);
			void clear();
			size_t getUsed();
#ifdef DEBUGRINGBUFFER
			int print();
			int print(int hexwith);
#endif

	};
}

#endif
