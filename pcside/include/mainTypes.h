#ifndef MAINTYPES
#define MAINTYPES

#include <addrlist.h>
#include <espsiteTypes.h>
#include <devices.h>
#include <stdint.h>
#include <helpertypes.h>


struct state {
	addrlist::root *addrRoot;
	helper::thslist<devices::device*> *devices;
};
typedef struct state *Pstate;

#endif
