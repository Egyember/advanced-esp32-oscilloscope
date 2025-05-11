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
namespace devices{

	//todo: refactor to std::list + mutex insted of costume ll

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
}
#endif
