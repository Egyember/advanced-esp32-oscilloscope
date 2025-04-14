#ifndef DEVICES
#define DEVICES
#include <addrlist.h>
#include <stddef.h>
#include <stdint.h>
#include "espsiteTypes.h"
#include <pthread.h>

#ifndef BUFFERMULTIPLIER
#define BUFFERMULTIPLIER 8
#endif
struct ringbuffer{
	unsigned char *bufferStart;
	size_t bufferLength;
	pthread_mutex_t writeLock;
	unsigned char *wrPrt;
	pthread_mutex_t readLock;
	unsigned char *rdPrt;
};

int initBuffer(struct ringbuffer *buffer, size_t size);
int readBuffer(struct ringbuffer *buffer, unsigned char *dest, size_t size);
int writeBuffer(struct ringbuffer *buffer, unsigned char *dest, size_t size);

struct device {

	/**
	 * addrllroot must be locked for reading manually befor reading or writeing.
	 * At least the next, previus,lastseen filds
	 */
	struct scopeConf config;
	struct sockaddr *address;
	int fd;

	/**
	 * buffer size is based on the config
	 * config.sampleRate * (config.duration / 1000) * SOC_ADC_DIGI_RESULT_BYTES; 
	 * // SOC_ADC_DIGI_RESULT_BYTES = 2 byte = 16 bit
	 */
	struct ringbuffer *buffer; //array of buffer headers one per channel
	pthread_t reader;
	pthread_rwlock_t lock; //for changeing the pointers below
	struct device *next;
	struct device *prev;
};

int devices_append(struct device* to, struct device *dev);
int devices_disconnect(struct device *dev);

#endif
