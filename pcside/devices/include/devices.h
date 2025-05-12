#ifndef DEVICES
#define DEVICES
#include <addrlist.h>
#include <stddef.h>
#include <stdint.h>
#include "espsiteTypes.h"
#include <pthread.h>
#include <ringbuffer.h>
#include <helpertypes.h>
#include <vector>

#ifndef BUFFERMULTIPLIER
#define BUFFERMULTIPLIER 8
#endif
namespace devices{

	//todo: refactor to std::list + mutex insted of costume ll

class device {
	private:
		/**
		 * addrllroot must be locked for reading manually befor reading or writeing.
		 * At least the next, previus,lastseen filds
		 */
		struct sockaddr *address;
		int fd;
		/**
		 * buffer size is based on the config
		 * config.sampleRate * (config.duration / 1000) * SOC_ADC_DIGI_RESULT_BYTES; 
		 * // SOC_ADC_DIGI_RESULT_BYTES = 2 byte = 16 bit
		 */
		pthread_t reader;
		static void *readerfunc(device *dev);

		struct scopeConf config;
	public:
		device(struct scopeConf config, addrlist::root *root, struct sockaddr *address,
			       socklen_t address_len);
		~device();
		//int devices_disconnect();
		std::vector< ringbuffers::ringbuffer> buffer; //array of buffer headers one per channel
};

}
#endif
