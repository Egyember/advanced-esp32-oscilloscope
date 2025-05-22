#include <assert.h>
#include <cstddef>
#include <pthread.h>
#include <ringbuffer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#ifdef NDEBUG
#define DEBUGRINGBUFFER
#endif
#if defined(NDEBUG) || defined(DEBUGRINGBUFFER)
#include <iostream>
#endif

using namespace ringbuffers;

ringbuffer::ringbuffer(size_t size) {
// int initBuffer(struct ringbuffer *buffer, size_t size){
#ifdef DEBUGRINGBUFFER
	std::cout << "ring buffer init started for " << size << "\n";
#endif
	this->_data = new std::vector<unsigned char>;
	this->_data->reserve(size);
	this->_data->resize(size);
	std::fill(this->_data->begin(), this->_data->end(), 0);
	pthread_mutex_init(&this->lock, NULL);
	this->maxindex = size - 1;
	this->readindex = 0;
	this->writeindex = 0;
	this->empty = true;
#ifdef DEBUGRINGBUFFER
	std::cout << "ring buffer init finished for " << size << "\n";
#endif
};

ringbuffer::~ringbuffer() {
	delete this->_data;
	pthread_mutex_destroy(&this->lock);
}

int ringbuffer::readBuffer(unsigned char *dest, size_t size) {
	if(dest == NULL) {
		if(size == 0) {
			return 0;
		} else {
			return -1;
		}
	}
#ifdef DEBUGRINGBUFFER
	std::cout << "ring buffer read started: " << size << "\n";
#endif
	pthread_mutex_lock(&this->lock);
	if(empty) {
#ifdef DEBUGRINGBUFFER
		std::cout << "ring buffer empty read returned: " << size << "\n";
#endif
		pthread_mutex_unlock(&this->lock);
		return 0;
	}
	size_t canread = writeindex - readindex;
	size_t canreadsec = 0;
	size_t writen = 0;
	if(writeindex < readindex) {
		canreadsec = writeindex - (size - canread);
	}
	if(canread > size) {
#ifdef DEBUGRINGBUFFER
		std::cout << "debug\n";
#endif
		memcpy(dest, &_data->data()[readindex], size);
		writen += size;
		readindex += size;
	} else {
#ifdef DEBUGRINGBUFFER
		std::cout << "debug2 " << canread << "\n";
#endif
		memcpy(dest, &_data->data()[readindex], canread);
		writen += canread;
		memcpy(&dest[canread], _data->data(), canreadsec);
		writen += canreadsec;
		readindex = canreadsec;
	}
	pthread_mutex_unlock(&this->lock);
#ifdef DEBUGRINGBUFFER
	std::cout << "ring buffer read finished: " << writen << "\n";
#endif
	return writen;
};

size_t ringbuffer::writeBuffer(unsigned char *src, size_t size) {
#ifdef DEBUGRINGBUFFER
	std::cout << "ring buffer write started: " << size << "\n";
#endif
	if(empty) {
		if(size > 0) {
			empty = false;
		}
	}
	if(size > maxindex) {
		size = maxindex;
	};
	pthread_mutex_lock(&this->lock);
	size_t writeable = maxindex - writeindex;
	if(size < writeable) {
		memcpy(&_data->data()[writeindex], src, size);
		if(writeindex >= readindex) {
			writeindex += size;
		} else {
			writeindex += size;
			readindex = writeindex;
		}
	} else {
		size_t newerindex = (writeindex + size) % maxindex;
		if(!(newerindex < readindex && readindex < writeindex)) {
			readindex = newerindex;
		}
		memcpy(&_data->data()[writeindex], src, writeable);
		size_t secundwr = size - writeable;
		memcpy(_data->data(), src, secundwr);
		writeindex = newerindex;
	};
	pthread_mutex_unlock(&this->lock);
#ifdef DEBUGRINGBUFFER
	std::cout << "ring buffer write finished: " << size << "\n";
#endif
	return size;
};
