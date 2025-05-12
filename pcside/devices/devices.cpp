#include "addrlist.h"
#include <assert.h>
#include <devices.h>
#include <espsiteTypes.h>
#include <math.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ringbuffer.h>
#include <vector>



int writeConfig(int fd, struct scopeConf *config) {
	assert(config);
	unsigned char buffer[(sizeof(uint32_t) * 2 + sizeof(uint8_t))];
	buffer[0] = config->channels;
	*((uint32_t *)&buffer[1]) = htonl(config->duration);
	*((uint32_t *)&buffer[sizeof(uint32_t) + 1]) = htonl(config->sampleRate);
	return write(fd, buffer, (sizeof(uint32_t) * 2 + sizeof(uint8_t))) < 0 ? -1 : 0;
};

void* devices::device::readerfunc(devices::device *dev) {
	unsigned char *tempBuffer = new unsigned char[(int)std::floor(dev->config.sampleRate * ((double)dev->config.duration / 1000.0) * dev->config.channels)];
	std::vector<unsigned char *> prechanbuffs;
	std::vector<size_t>perchanbuffoffset;
	for(int i = 0; i < dev->config.channels; i++) {
		prechanbuffs.push_back(new unsigned char[(int)(std::floor(dev->config.sampleRate * ((double)dev->config.duration / 1000.0)))]);
		perchanbuffoffset.push_back(0);
	}
	uint8_t lastchannel = 0;
	while(true) {
		int readedData = read(dev->fd, tempBuffer,
				      (int)std::floor(dev->config.sampleRate * ((double)dev->config.duration / 1000.0) *
						 dev->config.channels));
		if(readedData < 0) {
			printf("read failed");
			break;
		};
		//memcopy?
		for(int i = 0; i < readedData; i++) {
			if(lastchannel > (this->config.channels - 1)) {
				lastchannel = 0;
			}
			prechanbuffs[lastchannel][perchanbuffoffset[lastchannel]++] = tempBuffer[i]; //
			lastchannel++;
		}
		for(int i = 0; i < dev->config.channels; i++) {
			dev->buffer[i].writeBuffer(prechanbuffs[i], perchanbuffoffset[i]);
		}
		for(int i = 0; i < dev->config.channels; i++) {
			perchanbuffoffset[i] = 0;
		}
	}
	delete[] tempBuffer;

	for(int i = 0; i < dev->config.channels; i++) {
		prechanbuffs.clear();
	}
	return NULL;
};

devices::device::device(struct scopeConf config, addrlist::root *root, struct sockaddr *address,
			       socklen_t address_len) {
	assert(root);
	assert(address);
	memcpy(&this->config, &config, sizeof(struct scopeConf));
	// init ring buffer here
	for(int i = 0; i < config.channels; i++) {
		ringbuffers::ringbuffer tbuff(floor(config.sampleRate * ((double)config.duration / 1000.0)) * BUFFERMULTIPLIER);
		buffer.push_back(tbuff);
	}

	this->address = address;
	root->connect(address);
	this->fd = socket(AF_INET, SOCK_STREAM, 0);
	if(connect(this->fd, address, address_len) != 0) {
		close(this->fd);
		buffer.clear();
		throw "connection error";
		return;
	};
	if(writeConfig(this->fd, &this->config) != 0) {
		close(this->fd);
		buffer.clear();
		throw "connection error";
		return;
	}

	pthread_create(&this->reader, NULL, (void *(*)(void*))&this->readerfunc, this);
	return;
};

/*
int devices_append(struct device *to, struct device *dev) {
	assert(to);
	assert(dev);
	struct device *last = NULL;
	{
		pthread_rwlock_rdlock(&to->lock);
		struct device *next = to->next;
		pthread_rwlock_unlock(&to->lock);
		while(1) {
			pthread_rwlock_rdlock(&next->lock);
			if(next->next == NULL) {
				pthread_rwlock_unlock(&next->lock);
				pthread_rwlock_wrlock(&next->lock);
				last = next;
				break;
			}else{
				struct device *nextnext = next->next;
				pthread_rwlock_unlock(&next->lock);
				next = nextnext;
			}	
		}
	}
	last->next = dev;
	dev->prev = last;
	pthread_rwlock_unlock(&last->lock);
	return 0;
};
*/

devices::device::~device(){
//int devices_disconnect(struct device *dev){
	printf("not implemented disconnect, expect memory leak\n");
	return;
};
