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
}

#endif
