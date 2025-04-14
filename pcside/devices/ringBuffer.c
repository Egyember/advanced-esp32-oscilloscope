#include <assert.h>
#include <devices.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int initBuffer(struct ringbuffer *buffer, size_t size){
	if (buffer == NULL) {
		return -1;
	}
	if (size<0) {
		return -1;
	}
	buffer->bufferLength = size;
	buffer->bufferStart = malloc(size);
	if (buffer->bufferStart == NULL) {
		printf("out of memory");
		return -1;
	}
	pthread_mutex_init(&(buffer->readLock), NULL);
	pthread_mutex_init(&(buffer->writeLock), NULL);
	buffer->rdPrt = buffer->bufferStart;
	buffer->wrPrt = buffer->bufferStart;
	return 0;
};
int readBuffer(struct ringbuffer *buffer, unsigned char *dest, size_t size){
	pthread_mutex_lock(&buffer->readLock);
	pthread_mutex_lock(&buffer->writeLock);
	size_t avalable;
	if( buffer->wrPrt > buffer->rdPrt){
		avalable = buffer->wrPrt - buffer->rdPrt;
	}else{
		avalable = buffer->bufferLength - (buffer->wrPrt - buffer->rdPrt);
	}
	pthread_mutex_unlock(&buffer->writeLock);
	if (size>avalable) {
		size = avalable;
	};
	int overflow = (buffer->rdPrt+size) - (buffer->bufferStart+buffer->bufferLength);
	if(overflow>0){
		memcpy(dest, buffer->rdPrt, size - overflow);
		memcpy(dest+(size - overflow), buffer->bufferStart, overflow);
		buffer->rdPrt = buffer->bufferStart+overflow;
	}else{
		memcpy(dest, buffer->rdPrt, size);
		buffer->rdPrt += size;
	}
	pthread_mutex_unlock(&buffer->readLock);
	return size;
};

int writeBuffer(struct ringbuffer *buffer, unsigned char *src, size_t size){
	pthread_mutex_lock(&buffer->writeLock);
	pthread_mutex_lock(&buffer->readLock);
	size_t avalable;
	if( buffer->wrPrt < buffer->rdPrt){
		avalable = buffer->wrPrt - buffer->rdPrt;
	}else{
		avalable = buffer->bufferLength - (buffer->wrPrt - buffer->rdPrt);
	}
	int overflow =  (buffer->rdPrt +size) - (buffer->bufferStart+buffer->bufferLength);
	if (size > avalable) {
		if (overflow > 0) {
			buffer->rdPrt = buffer->bufferStart + overflow;
		}else{
			buffer->rdPrt += size;
		}
	}
	pthread_mutex_unlock(&buffer->readLock);
	if(overflow>0){
		memcpy(buffer->wrPrt, src, size - overflow);
		memcpy(buffer->bufferStart, src+size - overflow, overflow);
		buffer->wrPrt = buffer->bufferStart+overflow;
	}else{
		memcpy(buffer->wrPrt, src, size);
		buffer->wrPrt += size;
	}
	pthread_mutex_unlock(&buffer->writeLock);
	return size;
};

