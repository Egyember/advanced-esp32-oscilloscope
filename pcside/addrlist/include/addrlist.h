#ifndef addrlist
#define addrlist
#include <stdbool.h>
#include <sys/socket.h>
#include <time.h>
#include <pthread.h>

typedef struct addrllroot {
	pthread_rwlock_t lock;
	struct addrll *next;
} addrllroot;
typedef struct addrll {
	struct sockaddr addr;
	time_t lastseen;
	bool conneted;
	struct addrll *next;
	struct addrll *prev;
} addrll;

addrllroot *addrll_init();

int addrl_update(addrllroot *addrll, struct sockaddr addr);
int addrll_deletOld(addrllroot *addrll);
int addrll_connect(addrllroot *root, struct sockaddr *taddress);
int addrll_disconnect(addrllroot *root, struct sockaddr *taddress);
int addrll_lenth(addrllroot *root);
void *scanForEsp(addrllroot *root);
#endif
