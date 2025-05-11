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

int writeConfig(int fd, struct scopeConf *config) {
	assert(config);
	unsigned char buffer[(sizeof(uint32_t) * 2 + sizeof(uint8_t))];
	buffer[0] = config->channels;
	*((uint32_t *)&buffer[1]) = htonl(config->duration);
	*((uint32_t *)&buffer[sizeof(uint32_t) + 1]) = htonl(config->sampleRate);
	return write(fd, buffer, (sizeof(uint32_t) * 2 + sizeof(uint8_t))) < 0 ? -1 : 0;
};

void *reader(struct device *dev) {
	assert(dev);
	unsigned char *tempBuffer =
	    malloc((int)floor(dev->config.sampleRate * ((double)dev->config.duration / 1000.0) * dev->config.channels));
	unsigned char *perchanbuffs[dev->config.channels];
	size_t perchanbuffoffset[dev->config.channels];
	for(int i = 0; i < dev->config.channels; i++) {
		perchanbuffs[i] = malloc(floor(dev->config.sampleRate * ((double)dev->config.duration / 1000.0)));
		perchanbuffoffset[i] = 0;
	}
	uint8_t lastchannel = 0;
	while(true) {
		int readedData = read(dev->fd, tempBuffer,
				      (int)floor(dev->config.sampleRate * ((double)dev->config.duration / 1000.0) *
						 dev->config.channels));
		if(readedData < 0) {
			printf("read failed");
			break;
		};
		for(int i = 0; i < readedData; i++) {
			if(lastchannel > (dev->config.channels - 1)) {
				lastchannel = 0;
			}
			perchanbuffs[lastchannel][perchanbuffoffset[lastchannel]++] = tempBuffer[i]; //
			lastchannel++;
		}
		for(int i = 0; i < dev->config.channels; i++) {
			writeBuffer(&dev->buffer[i], perchanbuffs[i], perchanbuffoffset[i]);
		}
		for(int i = 0; i < dev->config.channels; i++) {
			perchanbuffoffset[i] = 0;
		}
	}
	free(tempBuffer);

	for(int i = 0; i < dev->config.channels; i++) {
		free(perchanbuffs[i]);
	}
	return NULL;
};

struct device *devices_connect(struct scopeConf config, addrllroot *root, struct sockaddr *address,
			       socklen_t address_len) {
	assert(root);
	assert(address);
	struct device *dev = malloc(sizeof(struct device));
	if(dev == NULL) {
		printf("out of memory\n");
		return NULL;
	}
	memcpy(&(dev->config), &config, sizeof(struct scopeConf));
	// init ring buffer here
	dev->buffer = malloc(sizeof(struct ringbuffer) * config.channels);
	for(int i = 0; i < config.channels; i++) {
		initBuffer(&(dev->buffer[i]),
			   floor(config.sampleRate * ((double)config.duration / 1000.0)) * BUFFERMULTIPLIER);
	}
	dev->address = address;
	addrll_connect(root, address);
	dev->fd = socket(AF_INET, SOCK_STREAM, 0);
	if(connect(dev->fd, address, address_len) != 0) {
		close(dev->fd);
		free(dev->buffer);
		free(dev);
		return NULL;
	}
	if(writeConfig(dev->fd, &dev->config) != 0) {
		close(dev->fd);
		free(dev->buffer);
		free(dev);
		return NULL;
	}

	pthread_create(&dev->reader, NULL, (void *(*)(void *))reader, dev);
	dev->next = NULL;
	dev->prev = NULL;
	pthread_rwlock_init(&dev->lock, NULL);

	return dev;
};

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

int devices_disconnect(struct device *dev){
	printf("not implemented\n");
	return 0;
};
