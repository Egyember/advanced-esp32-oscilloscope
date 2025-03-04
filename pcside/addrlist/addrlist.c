#include "addrlist.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAXTIME 120 // 2 minutes

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
