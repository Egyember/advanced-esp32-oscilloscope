#ifndef RINGBUFFER
#define RINGBUFFER
#include <pthread.h>
namespace ringbuffers {

	class ringbuffer{
		private:
			unsigned char *bufferStart;
			size_t bufferLength;
			pthread_mutex_t writeLock;
			unsigned char *wrPrt;
			pthread_mutex_t readLock;
			unsigned char *rdPrt;
		public:
			//int initBuffer(size_t size);
			ringbuffer(size_t size);
			~ringbuffer();
			int readBuffer(unsigned char *dest, size_t size);
			int writeBuffer(unsigned char *dest, size_t size);

	};
}

#endif
