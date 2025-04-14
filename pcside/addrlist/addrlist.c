#include "addrlist.h"
#include <bits/types/struct_timeval.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#define MAXTIME 60 // 1 minutes

addrllroot *addrll_init(){
	addrllroot *ret = malloc(sizeof(addrllroot));
	ret->next = NULL;
	pthread_rwlock_init(&(ret->lock), NULL);
	return ret;
};

int addrll_update(addrllroot *root, struct sockaddr addr) {
	pthread_rwlock_rdlock(&(root->lock));
	addrll *nodeAddr = root->next;
	bool found = false;
	while(nodeAddr != NULL) {
		if(memcmp(&(nodeAddr->addr), &addr, sizeof(nodeAddr->addr)) == 0) {
			time_t now;
			time(&now);
			pthread_rwlock_unlock(&(root->lock));
			pthread_rwlock_wrlock(&(root->lock));
			nodeAddr->lastseen = now;
			pthread_rwlock_unlock(&(root->lock));
			pthread_rwlock_rdlock(&(root->lock));
			found = true;
			break;
		}
		nodeAddr = nodeAddr->next;
	}
	if(!found) {
		addrll *newNode = malloc(sizeof(addrll));
		memset(newNode, '\0', sizeof(addrll));
		memcpy(&(newNode->addr), &addr, sizeof(struct sockaddr));
		if(root->next == NULL) {
			root->next = newNode;
		} else {
			nodeAddr = root->next;
			while(nodeAddr != NULL) { //this can be avoided if I add a lase fild to the root node
				if(nodeAddr->next == NULL) {
					pthread_rwlock_unlock(&(root->lock));
					pthread_rwlock_wrlock(&(root->lock));
					nodeAddr->next = newNode;
					newNode->prev = nodeAddr;
					pthread_rwlock_unlock(&(root->lock));
					pthread_rwlock_rdlock(&(root->lock));
					break;
				}
				nodeAddr = nodeAddr->next;
			}
		}
	}
	pthread_rwlock_unlock(&(root->lock));
	return 0;
};

int addrll_deletOld(addrllroot *root) {
	if (root == NULL) {
		printf("NULL arg\n");
		exit(-1);
	}
	pthread_rwlock_rdlock(&(root->lock));
	addrll *addr = root->next;
	while(addr != NULL) {
		time_t now;
		time(&now);
		if(addr->lastseen - now > MAXTIME && !addr->conneted) {
			pthread_rwlock_unlock(&(root->lock));
			pthread_rwlock_wrlock(&(root->lock));
			addr->prev->next = addr->next;
			free(addr);
			pthread_rwlock_unlock(&(root->lock));
			pthread_rwlock_rdlock(&(root->lock));
		}
		addr = addr->next;
	}
	pthread_rwlock_unlock(&(root->lock));
	return 0;
};

int addrll_connect(addrllroot *root, struct sockaddr *taddress) {
	if (root == NULL) {
		printf("NULL arg\n");
		exit(-1);
	}
	pthread_rwlock_rdlock(&(root->lock));
	addrll *addr = root->next;
	while(addr != NULL) {
		if(memcmp(addr, taddress, sizeof(struct sockaddr))== 0) {
			pthread_rwlock_unlock(&(root->lock));
			pthread_rwlock_wrlock(&(root->lock));
			addr->conneted = true;
			pthread_rwlock_unlock(&(root->lock));
			pthread_rwlock_rdlock(&(root->lock));
		}
		addr = addr->next;
	}
	pthread_rwlock_unlock(&(root->lock));
	return 0;
};

int addrll_disconnect(addrllroot *root, struct sockaddr *taddress) {
	if (root == NULL) {
		printf("NULL arg\n");
		exit(-1);
	}
	pthread_rwlock_rdlock(&(root->lock));
	addrll *addr = root->next;
	while(addr != NULL) {
		if(memcmp(addr, taddress, sizeof(struct sockaddr))== 0) {
			pthread_rwlock_unlock(&(root->lock));
			pthread_rwlock_wrlock(&(root->lock));
			addr->conneted = false;
			pthread_rwlock_unlock(&(root->lock));
			pthread_rwlock_rdlock(&(root->lock));
		}
		addr = addr->next;
	}
	pthread_rwlock_unlock(&(root->lock));
	return 0;
};


int addrll_lenth(addrllroot *root){
	if (root == NULL) {
		printf("NULL arg\n");
		exit(-1);
	}
	int len = 0;
	pthread_rwlock_rdlock(&(root->lock));
	addrll *addr = root->next;
	while(addr != NULL) {
		len++;
		addr = addr->next;
	}
	pthread_rwlock_unlock(&(root->lock));
	return len;
};


//todo fix buffer overflow
const static char *search = "oscilloscope here";
void *scanForEsp(addrllroot *root) {
	int soc = socket(AF_INET, SOCK_DGRAM, 0);
	if(soc < 0) {
		printf("Unable to create socket errno: %d", errno);
	}

	static const struct timeval timeout= {
		.tv_sec = MAXTIME/2,
		.tv_usec = 0,
	};	
	setsockopt(soc, SOL_SOCKET, SO_RCVTIMEO, (const void *) &timeout, sizeof(timeout));
	struct sockaddr_in localAddr = {.sin_addr = INADDR_ANY, .sin_port = htons(40000), .sin_family = AF_INET};
	if(bind(soc, (const struct sockaddr *)&localAddr, sizeof(localAddr)) < 0) {
		printf("can't bind to socket\n");
		exit(-1);
	};
	char buffer[strlen(search)];
	memset(buffer, '\0', strlen(search));
	while(true) {
		struct sockaddr addr;
		socklen_t addrlen = sizeof(addr);
		int aread = recvfrom(soc, &buffer, sizeof(buffer), 0, &addr, &addrlen);
		if (aread <0) {
			printf("read failed err: %d\n", aread);
			addrll_deletOld(root);
			continue;
	//		exit(-1);
		}
		if (aread == 0) {
			addrll_deletOld(root);
			continue;
		}
		// itt V
		if(strncmp(buffer, search, strlen(search)) == 0) {
			addrll_update(root, addr);
		};
		addrll_deletOld(root);
	}
	return NULL;
}
