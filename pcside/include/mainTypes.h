#ifndef MAINTYPES
#define MAINTYPES

#include <addrlist.h>
#include <espsiteTypes.h>
#include <devices.h>
#include <stdint.h>


struct state {
	addrlist::root *addrRoot;
	struct device* devices;
};
typedef struct state *Pstate;

#endif
