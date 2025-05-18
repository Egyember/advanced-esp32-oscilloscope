#ifndef RINGBUFFER
#define RINGBUFFER
#include <pthread.h>
namespace ringbuffers {

	class ringbuffer{
		private:
			unsigned char *bufferStart;
			size_t bufferLength;
			unsigned char *wrPrt;
			unsigned char *rdPrt;
			pthread_rwlock_t lock;
			pthread_mutex_t locklock;
			size_t freeToWrite();
			size_t readable();
			void rdlock();
			void wrlock();
			void upgrade();
		public:
			//int initBuffer(size_t size);
			ringbuffer(size_t size);
			~ringbuffer();
			int readBuffer(unsigned char *dest, size_t size);
			int writeBuffer(unsigned char *dest, size_t size);

	};
}

#endif
