#include <assert.h>
#include <pthread.h>
#include <ringbuffer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUGRINGBUFFER
#ifdef NDEBUG
#define DEBUGRINGBUFFER
#define DEBUGLOCKUPGRAGE
#endif
#if defined(NDEBUG) || defined(DEBUGRINGBUFFER) || defined (DEBUGLOCKUPGRAGE)
#include <iostream>
#endif

using namespace ringbuffers;

void ringbuffer::rdlock(){
	pthread_rwlock_rdlock(&this->lock);
}
void ringbuffer::wrlock(){
	pthread_rwlock_wrlock(&this->lock);
}
/**
 * pthread dosn't contain mechanisms to upgradi a read lock to a write lock
 * without race conditions.
 **/

ringbuffer::ringbuffer(size_t size) {
// int initBuffer(struct ringbuffer *buffer, size_t size){
#ifdef DEBUGRINGBUFFER
	std::cout << "ring buffer init started for " << size << "\n";
#endif
	this->bufferLength = size;
	this->bufferStart = new unsigned char[size];
	memset(this->bufferStart, 0, size);
	pthread_rwlock_init(&(this->lock), NULL);
	pthread_mutex_init(&this->locklock, NULL);
	this->rdPrt = this->bufferStart;
	this->wrPrt = this->bufferStart;
#ifdef DEBUGRINGBUFFER
	std::cout << "ring buffer init finished for " << size << "\n";
#endif
};

ringbuffer::~ringbuffer() { // posible doublefree
	delete[] this->bufferStart;
	pthread_rwlock_destroy(&this->lock);
	pthread_mutex_destroy(&this->locklock);
}

size_t ringbuffer::freeToWrite() {
	//rdlock();
	size_t a =
	    (this->wrPrt > this->rdPrt) ? this->wrPrt - this->rdPrt : this->bufferLength - (this->wrPrt - this->rdPrt);
	//pthread_rwlock_unlock(&this->lock);
	return a;
};
size_t ringbuffer::readable() {
	rdlock();
	size_t a = this->bufferLength - this->freeToWrite();
	pthread_rwlock_unlock(&this->lock);
	return a;
};

int ringbuffer::readBuffer(unsigned char *dest, size_t size) {
#ifdef DEBUGRINGBUFFER
	std::cout << size << " has been attempted to read\n";
#endif
	rdlock();
	size_t avalable = this->readable();
	pthread_rwlock_unlock(&this->lock);
	wrlock();
#ifdef DEBUGRINGBUFFER
	std::cout << "avalable: " << avalable << "\n";
#endif

	if(size > avalable) {
		size = avalable;
	};
	int overflow = (this->rdPrt + size) - (this->bufferStart + this->bufferLength);
	if(overflow > 0) {
		memcpy(dest, this->rdPrt, size - overflow);
		memcpy(dest + (size - overflow), this->bufferStart, overflow);
		this->rdPrt = this->bufferStart + overflow;
	} else {
		memcpy(dest, this->rdPrt, size);
		this->rdPrt += size;
	}
	pthread_rwlock_unlock(&this->lock);

#ifdef DEBUGRINGBUFFER
	std::cout << size << " hase been read from rign buffer\n";
#endif
	return size;
};

int ringbuffer::writeBuffer(unsigned char *src, size_t size) {
	wrlock();
	size_t avalable = this->freeToWrite();
	int overflow = (this->rdPrt + size) - (this->bufferStart + this->bufferLength);
	if(size > avalable) {
		if(overflow > 0) {
			this->rdPrt = this->bufferStart + overflow;
		} else {
			this->rdPrt += size;
		}
	}
	if(overflow > 0) {
		memcpy(this->wrPrt, src, size - overflow);
		memcpy(this->bufferStart, src + size - overflow, overflow);
		this->wrPrt = this->bufferStart + overflow;
	} else {
		memcpy(this->wrPrt, src, size);
		this->wrPrt += size;
	}
	pthread_rwlock_unlock(&this->lock);
#ifdef DEBUGRINGBUFFER
	std::cout << size << " hase been writen to rign buffer\n";
#endif
	return size;
};
