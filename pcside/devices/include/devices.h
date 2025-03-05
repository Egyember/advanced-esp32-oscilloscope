#ifndef DEVICES
#define DEVICES
#include <addrlist.h>
#include <stdint.h>
#include <espsiteTypes.h>
#include <pthread.h>
struct device {

	/**
	 * addrllroot must be locked for reading manually befor reading or writeing.
	 * At least the next, previus,lastseen filds
	 */
	addrll *discovered;
	struct scopeConf config;

	/**
	 * buffer size is based on the config
	 * config.sampleRate * (config.duration / 1000) * SOC_ADC_DIGI_RESULT_BYTES; 
	 * // SOC_ADC_DIGI_RESULT_BYTES = 2 byte = 16 bit
	 */
	uint16_t *buffer; //replace with ring buffer
	pthread_t reader;
	struct device *next;
};

typedef struct device *devices;

struct device *devices_connect(struct scopeConf config, addrll addres);
int devices_append(devices devices, struct device dev);
int devices_disconnect(devices devices, struct device dev);
#endif
