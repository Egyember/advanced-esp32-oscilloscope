#ifndef MAINTYPES
#define MAINTYPES

#include <addrlist.h>
#include <espsiteTypes.h>
#include <devices.h>
#include <list>
#include <stdint.h>
#include <helpertypes.h>
#include <recorder.h>
#include <raylib.h>


class state {
	public:
		
	addrlist::root addrRoot;
	helper::thwraper<std::list<devices::device *>> *devices;
	record::recorederstate recordstate;
	bool addDialog = false;
};

#endif
