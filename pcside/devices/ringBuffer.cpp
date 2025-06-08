#include <assert.h>
#include <cstddef>
#include <cstdlib>
#include <pthread.h>
#include <ringbuffer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#define DEBUGRINGBUFFEROVERLOAD
#ifdef NDEBUG
#define DEBUGRINGBUFFER
#define DEBUGRINGBUFFEROVERLOAD
#endif
#if defined(NDEBUG) || defined(DEBUGRINGBUFFER) || defined(DEBUGRINGBUFFEROVERLOAD)

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


#ifdef DEBUGRINGBUFFER
int ringbuffer::print(){
	return print(64);
};
int ringbuffer::print(int hexwith){
	pthread_mutex_lock(&this->lock);
	std::cout << "empty: " << empty << "\n";
	std::cout << "readindex: " << readindex << "\n";
	std::cout << "writeindex: " << writeindex << "\n";
	std::cout << "maxindex: " << maxindex << "\n";
	int i = 0;
	for (unsigned char c : *_data) {
		printf("%02x", c);
		if (++i == hexwith) {
			printf("\n");
			i=0;
		}
	}
	printf("\n");
	pthread_mutex_unlock(&this->lock);
	return 0;
};
#endif

size_t ringbuffer::readBuffer(unsigned char *dest, size_t size) {
	if(dest == NULL) {
		if(size == 0) {
			return 0;
		} else {
			return -1;
		}
	}
#ifdef DEBUGRINGBUFFER
	std::cout << "ring buffer read started: " << size << "\n";
	print();
#endif
	pthread_mutex_lock(&this->lock);
	if(empty) {
#ifdef DEBUGRINGBUFFER
		std::cout << "ring buffer empty read returned: " << size << "\n";
#endif
		pthread_mutex_unlock(&this->lock);
		return 0;
	}
	size_t canread = 0;
	size_t canreadsec = 0;
	size_t writen = 0;
	if(writeindex < readindex) {
		canread = maxindex - readindex;
		canreadsec = writeindex;
	}else {
		canread = writeindex - readindex;
	}
	if (canread == size || canread+canreadsec == size) {
		empty = true;
	}
	if(canread >= size) {
		memcpy(dest, &_data->data()[readindex], size);
#ifdef DEBUGRINGBUFFER
		memset(&_data->data()[readindex], 0, size);
#endif
		writen += size;
		readindex += size;
	} else if (canreadsec != 0) {
	
	 //posible bug with readindex in this branch
		memcpy(dest, &_data->data()[readindex], canread);
#ifdef DEBUGRINGBUFFER
		memset(&_data->data()[readindex], 0, canread);
#endif
		writen += canread;
		memcpy(&dest[canread], _data->data(), canreadsec);
#ifdef DEBUGRINGBUFFER
		memset(_data->data(), 0, canreadsec);
#endif
		writen += canreadsec;
		readindex = canreadsec;
	}else {
		memcpy(dest, &_data->data()[readindex], canread);
#ifdef DEBUGRINGBUFFER
		memset(&_data->data()[readindex], 0, canread);
#endif
		writen += canread;
		readindex += canread;
		
	}
	pthread_mutex_unlock(&this->lock);
#ifdef DEBUGRINGBUFFER
	std::cout << "ring buffer read finished: " << writen << "\n";
	print();
#endif
	return writen;
};

size_t ringbuffer::writeBuffer(unsigned char *src, size_t size) {
#ifdef DEBUGRINGBUFFER
	std::cout << "ring buffer write started: " << size << "\n";
	print();
#endif
	pthread_mutex_lock(&this->lock);
	if(size > maxindex+1) {
		size = maxindex+1;
	};
	size_t writeable = maxindex - writeindex;
	if(size < writeable) {
		memcpy(&_data->data()[writeindex], src, size);
		if(writeindex >= readindex) {
			writeindex += size;
		} else {
			writeindex += size;
			readindex = writeindex;
#if  defined(DEBUGRINGBUFFER) || defined(DEBUGRINGBUFFEROVERLOAD)
			std::cout << "pushed read index\n";
#endif
		}
	} else {
		size_t newerindex = (writeindex + size) % maxindex;
		if(!(newerindex < readindex && readindex < writeindex) && !empty) {
			readindex = newerindex;
#ifdef DEBUGRINGBUFFER
	std::cout << "pushed read index (at overroll)\n";
#endif
		}
		memcpy(&_data->data()[writeindex], src, writeable);
		size_t secundwr = size - writeable;
		memcpy(_data->data(), &src[writeable], secundwr);
		writeindex = newerindex;
	};
	if(empty) {
		if(size > 0) {
			empty = false;
		}
	}
	pthread_mutex_unlock(&this->lock);
#ifdef DEBUGRINGBUFFER
	std::cout << "ring buffer write finished: " << size << "\n";
	print();
#endif
	return size;
};


void ringbuffer::clear(){
	pthread_mutex_lock(&this->lock);
	readindex = 0;
	writeindex = 0;
	empty = true;
	pthread_mutex_unlock(&this->lock);
};
