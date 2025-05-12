#include <assert.h>
#include <ringbuffer.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef NDEBUG
#include <iostream>
#endif
using namespace ringbuffers;

ringbuffer::ringbuffer(size_t size){
//int initBuffer(struct ringbuffer *buffer, size_t size){
	this->bufferLength = size;
	this->bufferStart = new unsigned char[size];
	pthread_mutex_init(&(this->readLock), NULL);
	pthread_mutex_init(&(this->writeLock), NULL);
	this->rdPrt = this->bufferStart;
	this->wrPrt = this->bufferStart;
};

ringbuffer::~ringbuffer(){ //posible doublefree
	delete[] this->bufferStart;
	pthread_mutex_destroy(&this->readLock);
	pthread_mutex_destroy(&this->writeLock);
}

int ringbuffer::readBuffer(unsigned char *dest, size_t size){
	pthread_mutex_lock(&this->readLock);
	pthread_mutex_lock(&this->writeLock);
	size_t avalable;
	if( this->wrPrt > this->rdPrt){
		avalable = this->wrPrt - this->rdPrt;
	}else{
		avalable = this->bufferLength - (this->wrPrt - this->rdPrt);
	}
	pthread_mutex_unlock(&this->writeLock);
	if (size>avalable) {
		size = avalable;
	};
	int overflow = (this->rdPrt+size) - (this->bufferStart+this->bufferLength);
	if(overflow>0){
		memcpy(dest, this->rdPrt, size - overflow);
		memcpy(dest+(size - overflow), this->bufferStart, overflow);
		this->rdPrt = this->bufferStart+overflow;
	}else{
		memcpy(dest, this->rdPrt, size);
		this->rdPrt += size;
	}
	pthread_mutex_unlock(&this->readLock);

#ifdef NDEBUG
	std::cout << size << " hase been read from rign buffer\n";
#endif
	return size;
};

int ringbuffer::writeBuffer(unsigned char *src, size_t size){
	pthread_mutex_lock(&this->writeLock);
	pthread_mutex_lock(&this->readLock);
	size_t avalable;
	if( this->wrPrt < this->rdPrt){
		avalable = this->wrPrt - this->rdPrt;
	}else{
		avalable = this->bufferLength - (this->wrPrt - this->rdPrt);
	}
	int overflow =  (this->rdPrt +size) - (this->bufferStart+this->bufferLength);
	if (size > avalable) {
		if (overflow > 0) {
			this->rdPrt = this->bufferStart + overflow;
		}else{
			this->rdPrt += size;
		}
	}
	pthread_mutex_unlock(&this->readLock);
	if(overflow>0){
		memcpy(this->wrPrt, src, size - overflow);
		memcpy(this->bufferStart, src+size - overflow, overflow);
		this->wrPrt = this->bufferStart+overflow;
	}else{
		memcpy(this->wrPrt, src, size);
		this->wrPrt += size;
	}
	pthread_mutex_unlock(&this->writeLock);
#ifdef NDEBUG
	std::cout << size << " hase been writen to rign buffer\n";
#endif
	return size;
};

