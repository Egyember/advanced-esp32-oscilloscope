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
#include <vector>


class state {
	public:
		
	addrlist::root addrRoot;
	helper::thwraper<std::list<devices::device *>> *devices;
	//todo: this needs a thwraper
	record::recorederstate recordstate;
	helper::thwraper<std::vector<std::vector<int>>> offsets;
	struct {
		Rectangle popupBounds = {0};
		Rectangle popupScroll = {0};
		float slider = 0;
		bool folow = false;
		float zoom = 300; 

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

#warning todo: refactor this to use the struct everywhere
union deviderpoint{
	float array[2];
	struct {
		float x,y;
	}cord;
};
#endif
