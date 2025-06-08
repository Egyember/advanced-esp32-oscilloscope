#ifndef HELPER
#define HELPER
#include <list>
#include <pthread.h>
#include <stdio.h>

namespace helper {
template <typename T> class thwraper {
      private:
	pthread_rwlock_t lock; // for changeing the pointers below
      public:
	T _data;
	thwraper();
	~thwraper();
	int rdlock();
	int wrlock();
	int unlock();
};

template <typename T> thwraper<T>::thwraper() { pthread_rwlock_init(&this->lock, NULL); }

template <typename T> thwraper<T>::~thwraper() { pthread_rwlock_destroy(&this->lock); };

template <typename T> int thwraper<T>::rdlock() { return pthread_rwlock_rdlock(&lock); };

template <typename T> int thwraper<T>::wrlock() { return pthread_rwlock_wrlock(&lock); };

template <typename T> int thwraper<T>::unlock() { return pthread_rwlock_unlock(&lock); };

int hexdump(unsigned char *src, size_t len, unsigned int with = 16);
} // namespace helper

#endif
