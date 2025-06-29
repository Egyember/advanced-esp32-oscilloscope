#include "addrlist.h"
#include "samples.h"
#include <assert.h>
#include <devices.h>
#include <espsiteTypes.h>
#include <math.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <ringbuffer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <espsiteTypes.h>
//debug all if NDDEBUG
//
#ifdef NDEBUG
#define DEBUGINIT
#define DEBUGREAD
#define DEBUGSAMPLE
#endif
#if defined (NDEBUG) || defined (DEBUGSAMPLE) ||defined (DEBUGREAD) ||defined (DEBUGINIT) 
#include <arpa/inet.h>
#include <csignal>
#include <errno.h>
#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <helpertypes.h>
#endif
#ifdef DEBUGSAMPLE
#include <bitset>
#endif

#include <helpertypes.h>

//esp32 technical referace manual 29-5
#define MAXVOLT (3.120)
#define SAMPLEMAX (0b0000111111111111)
#define SAMPLEMASK (0b0000111111111111)
#define CHANMASK (~SAMPLEMASK)

int writeConfig(int fd, struct esp::scopeConf *config) {
	assert(config);
	unsigned char buffer[(sizeof(uint32_t) * 2 + sizeof(uint8_t))];
	buffer[0] = config->channels;
	*((uint32_t *)&buffer[1]) = htonl(config->duration);
	*((uint32_t *)&buffer[sizeof(uint32_t) + 1]) = htonl(config->sampleRate);
	return write(fd, buffer, (sizeof(uint32_t) * 2 + sizeof(uint8_t))) < 0 ? -1 : 0;
};

void *devices::device::readerfunc(devices::device *dev) {
#ifdef DEBUGREAD
	std::cout << "readerfunc started\n";
#endif
	unsigned char *tempBuffer = new unsigned char[(int)std::floor(
	    dev->config.sampleRate * ((double)dev->config.duration / 1000.0) * dev->config.channels)];
	while(true) {
		int readedData = read(dev->fd, tempBuffer,
				      (int)std::floor(dev->config.sampleRate * ((double)dev->config.duration / 1000.0) *
						      dev->config.channels));
		for (int i = 0; i < readedData/2; i += 1) {
			uint16_t *s = &((uint16_t*)tempBuffer)[i];
			uint16_t chan = (*s & CHANMASK) >> 12;
			if (chan >= 0 && chan <= dev->config.channels-1) {
				dev->buffer[chan]->writeBuffer((unsigned char *)s, sizeof(uint16_t));
			}else {
#ifdef DEBUGREAD
	std::cout << "hw bug detected\n";
#endif
			}
		
		}
	}
	delete[] tempBuffer;
	return NULL;
};

devices::device::device(struct esp::scopeConf config, addrlist::root *root, struct sockaddr *address,
			socklen_t address_len) {
	assert(root);
	assert(address);
	memcpy(&this->config, &config, sizeof(struct esp::scopeConf));
	// init ring buffer here
	for(int i = 0; i < config.channels; i++) {
		ringbuffers::ringbuffer *tbuff = new ringbuffers::ringbuffer(
		    floor(config.sampleRate * ((double)config.duration / 1000.0)) * BUFFERMULTIPLIER);
		buffer.push_back(tbuff);
	}

	this->address = address;
	root->connect(address);
	this->fd = socket(AF_INET, SOCK_STREAM, 0);
	bool ipv4 = false;
	if(address_len == 16) {
		ipv4 = true;
	};
	if(ipv4) {
		struct sockaddr_in *ip = (sockaddr_in *)address;
		ip->sin_port = ntohs(40001);
	} else { // ipv6 support
		struct sockaddr_in6 *ip = (sockaddr_in6 *)address;
		ip->sin6_port = ntohs(40001);
	}
	if(connect(this->fd, address, address_len) != 0) {

#ifdef DEBUGINIT
		std::cout << "connection failed\n";
		std::cout << "fd:" << this->fd << "\n";
		std::cout << "addrlen:" << address_len << "\n";
		std::cout << "ipv4 address:" << inet_ntoa(((sockaddr_in *)address)->sin_addr) << "\n";
		std::cout << "tcp port:" << htons(((sockaddr_in *)address)->sin_port) << "\n";
		std::cout << "errno:" << errno << "\n";
		std::cout << "errno string:" << strerror(errno) << "\n";
		std::raise(SIGINT);
#endif
		close(this->fd);
		buffer.clear();
		throw "connection error";
		return;
	};
	if(writeConfig(this->fd, &this->config) != 0) {
		close(this->fd);
		buffer.clear();
#ifdef DEBUGINIT
		std::cout << "can't send config\n";
#endif
		throw "connection error";
		return;
	}

	pthread_create(&this->reader, NULL, (void *(*)(void *)) & this->readerfunc, this);
	return;
};


devices::device::~device() {
	// int devices_disconnect(struct device *dev){
	printf("not implemented disconnect, expect memory leak\n");
	pthread_kill(this->reader, SIGKILL);
	return;
};

samples::sample devices::parseSample(uint16_t raw){
	uint16_t value = (raw & SAMPLEMASK);
	uint16_t chan = (raw & CHANMASK) >> 12;
	float volt = ((float)(value)/(float)SAMPLEMAX) * (float)MAXVOLT;
#ifdef DEBUGSAMPLE
				std::bitset<16> r(raw);
				std::cout << "raw value: " << r << "\n";
				std::cout << "raw number " << raw << "\n";
				std::bitset<16> v(value);
				std::cout << "value    : " << v << "\n";
				std::cout << "value num: " << value << "\n";
				std::bitset<16> c(chan);
				std::cout << "channel  : " << c << "\n";
				std::cout << "channel n: " << chan << "\n";
				std::bitset<16> cm(CHANMASK);
				std::cout << "channel m: " << cm << "\n";
#endif
	return samples::sample(volt, chan);
};
