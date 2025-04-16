#ifndef ADDRLIST
#define ADDRLIST
#include <ctime>
#include <pthread.h>
#include <string>
#include <sys/socket.h>
namespace addrlist {

struct addrllnode {
	struct sockaddr addr;
	time_t lastseen;
	bool conneted;
	struct addrllnode *next;
	struct addrllnode *prev;
	addrllnode();
	addrllnode(struct sockaddr addr);
};
class root {
      private:
	pthread_rwlock_t lock;
	struct addrllnode *next;
	char *search;
	size_t search_len;

      public:
	root();
	int update(struct sockaddr addr);
	int deletOld();
	int connect(struct sockaddr *taddress);
	int disconnect(struct sockaddr *taddress);
	int lenth();
	void *scanForEsp();
};

}; // namespace addrlist
#endif
