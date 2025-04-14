#ifndef ADDRLIST
#define ADDRLIST
#include <sys/socket.h>
#include <ctime>
#include <pthread.h>
namespace addrlist{

struct addrllnode {
	struct sockaddr addr;
	time_t lastseen;
	bool conneted;
	struct addrll *next;
	struct addrll *prev;
};
class root{
	private:
		pthread_rwlock_t lock;
		struct addrll *next;
	public:
		root();
		int addrl_update(struct sockaddr addr);
		int addrll_deletOld();
		int addrll_connect(struct sockaddr *taddress);
		int addrll_disconnect(struct sockaddr *taddress);
		int addrll_lenth();
		void *scanForEsp();
};


};
#endif
