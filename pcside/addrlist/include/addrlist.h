#ifndef ADDRLIST
#define ADDRLIST
#include <ctime>
#include <pthread.h>
#include <string>
#include <sys/socket.h>
#include <helpertypes.h>
namespace addrlist {

struct addrllnode {
	struct sockaddr addr;
	time_t lastseen;
	bool conneted;
	addrllnode();
	addrllnode(struct sockaddr addr);
inline bool operator==(const addrllnode& lhs);
};
class root {
      private:
	std::string search;
	pthread_t scanner;
	static void *scanForEsp(root *root);

      public:
	helper::thwraper<std::list<addrlist::addrllnode>> nodes;
	
	root(std::string search="");
	~root();
	int update(struct sockaddr addr);
	void deletOld();
	int connect(struct sockaddr *taddress);
	int disconnect(struct sockaddr *taddress);
	int lenth();
};

}; // namespace addrlist
#endif
