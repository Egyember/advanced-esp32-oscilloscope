#include <devices.h>
#include <espsiteTypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct device *devices_connect(struct scopeConf config, addrll addres) {
	struct device *dev = malloc(sizeof(struct device));
	if(dev == NULL) {
		printf("out of memory\n");
		return NULL;
	}
	memcpy(&(dev->config), &config, sizeof(struct scopeConf));
	//init ring buffer here
	//
	dev->next = NULL;
	
	return dev;
};
int devices_append(devices devices, struct device dev);
int devices_disconnect(devices devices, struct device dev);
