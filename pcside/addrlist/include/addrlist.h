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
	std::string search;
	pthread_t scanner;

      public:
	root();
	~root();
	int update(struct sockaddr addr);
	int deletOld();
	int connect(struct sockaddr *taddress);
	int disconnect(struct sockaddr *taddress);
	int lenth();
	static void *scanForEsp(root *root);
};

}; // namespace addrlist
#endif
