#include <bits/types/struct_timeval.h>
#include <csignal>
#include <cstring>
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

#ifdef NDEBUG

#include <iostream>

#endif

#define MAXTIME 60 // 1 minutes
#include <addrlist.h>
using namespace addrlist;

addrllnode::addrllnode() {
	memset(&addr, 0, sizeof(addr));
	lastseen = 0;
	conneted = false;
}

inline bool addrllnode::operator==(const addrllnode& lhs) { return std::memcmp(&lhs,this, sizeof(addrllnode)) == 0; };

addrllnode::addrllnode(struct sockaddr address) {
	addr = address;
	lastseen = 0;
	conneted = false;
}

int root::update(struct sockaddr addr) {
	nodes.rdlock();
	for (auto &n : nodes._data) {
		if(memcmp(&(n.addr), &addr, sizeof(n.addr)) == 0) {
			time_t now;
			time(&now);
			nodes.unlock();
			nodes.wrlock();
			n.lastseen = now;
			nodes.unlock();
			return 0;
		}
	}
	nodes.unlock();
	nodes.wrlock();
	addrllnode newNode(addr);
	nodes._data.push_back(newNode);
	nodes.unlock();
	return 0;
};

void root::deletOld() {
	nodes.rdlock();
	for (auto n : nodes._data) {
		time_t now;
		time(&now);
		if((n.lastseen - now > MAXTIME) && !n.conneted) {
			nodes.unlock();
			nodes.wrlock();
			nodes._data.remove(n);
			nodes.unlock();
			nodes.rdlock();
		}
	}
	nodes.unlock();
};

int root::connect(struct sockaddr *taddress) {
	nodes.rdlock();
	for (auto &n : nodes._data) {
		if(memcmp(&n.addr, taddress, sizeof(struct sockaddr)) == 0) {
			nodes.unlock();
			nodes.wrlock();
			n.conneted = true;
			nodes.unlock();
			return 0;
		}
	}
	nodes.unlock();
	return -1;
};

int root::disconnect(struct sockaddr *taddress) {
	nodes.rdlock();
	for (auto &n : nodes._data) {
		if(memcmp(&n.addr, taddress, sizeof(struct sockaddr)) == 0) {
			nodes.unlock();
			nodes.wrlock();
			n.conneted = false;
			nodes.unlock();
			return 0;
		}
	}
	nodes.unlock();
	return -1;
};

int root::lenth() {
	int len = 0;
	nodes.rdlock();
	len = nodes._data.size();
	nodes.unlock();
	return len;
};

// todo fix buffer overflow
void *root::scanForEsp(root *root) {
	int soc = socket(AF_INET, SOCK_DGRAM, 0);
	if(soc < 0) {
		printf("Unable to create socket errno: %d", errno);
		pthread_exit((void *)-1);
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
	char buffer[root->search.length()+1]; // +null byte
	memset(buffer, '\0', root->search.length()+1);
#ifdef NDEBUG

	std::cout << "staring lissener\n";

#endif
	while(true) {
		struct sockaddr addr;
		socklen_t addrlen = sizeof(addr);
		int aread = recvfrom(soc, &buffer, sizeof(buffer), 0, &addr, &addrlen);
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
#ifdef NDEBUG

	std::cout << "got something\n";

#endif
		if(aread < 0) {
			printf("read failed err: %d\n", aread);
			root->deletOld();
			continue;
			//		exit(-1);
		}
		if(aread == 0) {
			root->deletOld();
			continue;
		}
		// itt V
		if(strncmp(buffer, root->search.c_str(), root->search.length()+1) == 0) {
			root->update(addr);
		};
		root->deletOld();
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	}
	return NULL;
}

root::root(std::string search) {
	this->search = search;
	void *(*fpointer)(void*)= (void* (*)(void*))&scanForEsp;
	pthread_create(&scanner, NULL, fpointer, this);
};

root::~root(){
	pthread_kill(scanner, SIGTERM);
};
