#include <bits/types/struct_timeval.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define MAXTIME 60 // 1 minutes
#include "addrlist.h"
using namespace addrlist;

addrllnode::addrllnode() {
	memset(&addr, 0, sizeof(addr));
	lastseen = 0;
	conneted = false;
	next = nullptr;
	prev = nullptr;
}

addrllnode::addrllnode(struct sockaddr address) {
	addr = address;
	lastseen = 0;
	conneted = false;
	next = nullptr;
	prev = nullptr;
}
root::root() {
	next = nullptr;
	pthread_rwlock_init(&(lock), NULL);
	search = "oscilloscope here";
};

int root::update(struct sockaddr addr) {
	pthread_rwlock_rdlock(&(this->lock));
	addrllnode *nodeAddr = this->next;
	bool found = false;
	while(nodeAddr != NULL) {
		if(memcmp(&(nodeAddr->addr), &addr, sizeof(nodeAddr->addr)) == 0) {
			time_t now;
			time(&now);
			pthread_rwlock_unlock(&(this->lock));
			pthread_rwlock_wrlock(&(this->lock));
			nodeAddr->lastseen = now;
			pthread_rwlock_unlock(&(this->lock));
			pthread_rwlock_rdlock(&(this->lock));
			found = true;
			break;
		}
		nodeAddr = nodeAddr->next;
	}
	if(!found) {
		addrllnode *newNode = new addrllnode(addr);
		if(next == nullptr) {
			next = newNode;
		} else {
			nodeAddr = next;
			while(nodeAddr != NULL) { // this can be avoided if I add a last fild to the root node
				if(nodeAddr->next == NULL) {
					pthread_rwlock_unlock(&lock);
					pthread_rwlock_wrlock(&lock);
					nodeAddr->next = newNode;
					newNode->prev = nodeAddr;
					pthread_rwlock_unlock(&lock);
					pthread_rwlock_rdlock(&lock);
					break;
				}
				nodeAddr = nodeAddr->next;
			}
		}
	}
	pthread_rwlock_unlock(&lock);
	return 0;
};

int root::deletOld() {
	pthread_rwlock_rdlock(&lock);
	addrllnode *addr = next;
	while(addr != NULL) {
		time_t now;
		time(&now);
		if(addr->lastseen - now > MAXTIME && !addr->conneted) {
			pthread_rwlock_unlock(&lock);
			pthread_rwlock_wrlock(&lock);
			addr->prev->next = addr->next;
			delete addr;
			pthread_rwlock_unlock(&lock);
			pthread_rwlock_rdlock(&lock);
		}
		addr = addr->next;
	}
	pthread_rwlock_unlock(&lock);
	return 0;
};

int root::connect(struct sockaddr *taddress) {
	pthread_rwlock_rdlock(&lock);
	addrllnode *addr = next;
	while(addr != NULL) {
		if(memcmp(addr, taddress, sizeof(struct sockaddr)) == 0) {
			pthread_rwlock_unlock(&lock);
			pthread_rwlock_wrlock(&lock);
			addr->conneted = true;
			pthread_rwlock_unlock(&lock);
			return 0;
		}
		addr = addr->next;
	}
	pthread_rwlock_unlock(&lock);
	return -1;
};

int root::disconnect(struct sockaddr *taddress) {
	pthread_rwlock_rdlock(&lock);
	addrllnode *addr = next;
	while(addr != NULL) {
		if(memcmp(addr, taddress, sizeof(struct sockaddr)) == 0) {
			pthread_rwlock_unlock(&lock);
			pthread_rwlock_wrlock(&lock);
			addr->conneted = false;
			pthread_rwlock_unlock(&lock);
			return 0;
		}
		addr = addr->next;
	}
	pthread_rwlock_unlock(&lock);
	return -1;
};

int root::lenth() {
	int len = 0;
	pthread_rwlock_rdlock(&lock);
	addrllnode *addr = next;
	while(addr != NULL) {
		len++;
		addr = addr->next;
	}
	pthread_rwlock_unlock(&lock);
	return len;
};

// todo fix buffer overflow
void *root::scanForEsp() {
	int soc = socket(AF_INET, SOCK_DGRAM, 0);
	if(soc < 0) {
		printf("Unable to create socket errno: %d", errno);
	}

	static const struct timeval timeout = {
	    .tv_sec = MAXTIME / 2,
	    .tv_usec = 0,
	};
	setsockopt(soc, SOL_SOCKET, SO_RCVTIMEO, (const void *)&timeout, sizeof(timeout));
	struct sockaddr_in localAddr = {
	    .sin_family = AF_INET,
	    .sin_port = htons(40000),
	    .sin_addr = INADDR_ANY,
	};
	if(bind(soc, (const struct sockaddr *)&localAddr, sizeof(localAddr)) < 0) {
		printf("can't bind to socket\n");
		exit(-1);
	};
	char buffer[search_len];
	memset(buffer, '\0', search_len);
	while(true) {
		struct sockaddr addr;
		socklen_t addrlen = sizeof(addr);
		int aread = recvfrom(soc, &buffer, sizeof(buffer), 0, &addr, &addrlen);
		if(aread < 0) {
			printf("read failed err: %d\n", aread);
			this->deletOld();
			continue;
			//		exit(-1);
		}
		if(aread == 0) {
			this->deletOld();
			continue;
		}
		// itt V
		if(strncmp(buffer, search.c_str(), search.length()) == 0) {
			this->update(addr);
		};
		deletOld();
	}
	return NULL;
}
