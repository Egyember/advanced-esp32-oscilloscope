#ifndef HELPER
#define HELPER
#include <list>
#include <pthread.h>

namespace helper{
template<typename T>class thslist{
	private:
		pthread_rwlock_t lock; //for changeing the pointers below
	public:
	std::list<T> list;
	thslist();
	~thslist();
	int rdlock();
	int wrlock();
	int unlock();
};


template<typename T> thslist<T>::thslist(){
	pthread_rwlock_init(&this->lock, NULL);
}

template<typename T> thslist<T>::~thslist(){
	pthread_rwlock_destroy(&this->lock);
}
}

#endif
