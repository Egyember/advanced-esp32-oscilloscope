#include <helpertypes.h>
#include <pthread.h>

using namespace helper;
template<typename T> thslist<T>::thslist(){
	pthread_rwlock_init(&this->lock, NULL);
}

template<typename T> thslist<T>::~thslist(){
	pthread_rwlock_destroy(&this->lock);
}
