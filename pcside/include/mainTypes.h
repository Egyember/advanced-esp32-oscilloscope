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
	struct {
		struct {
			bool addDialog = false;
			int addIndex = -1;
			int addActive = -1;
			struct sockaddr selectedAddress = {0};
			bool setupDialog = false;
			struct {
				bool chanEdit = false;
				bool sampEdit = false;
				bool duraEdit = false;
				char chan[33] = {0}; //0 inicialized becouse static, +1 for null byte
				char samp[33] = {0};
				char duration[33] = {0};
			}setup;

		}add;
	}gui;
};

#endif
