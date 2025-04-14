#ifndef MAINTYPES
#define MAINTYPES

#include <addrlist.h>
#include <espsiteTypes.h>
#include <devices.h>
#include <stdint.h>


struct state {
	addrllroot *addrRoot;
	struct device* devices;
};
typedef struct state *Pstate;

#endif
