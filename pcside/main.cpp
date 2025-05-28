#include "devices.h"
#include "helpertypes.h"
#include <addrlist.h>
#include <iostream>
#include <mainTypes.h>
#include <netinet/in.h>
#include <pthread.h>
#include <raygui.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <drawDevices.h>
#include <vector>

int main(void) {
	Pstate state = (Pstate)malloc(sizeof(struct state));
	state->addrRoot = new addrlist::root;
	state->devices = new helper::thslist<devices::device *>;
	InitWindow(0, 0, "teszt");
	int monitorCount = GetMonitorCount();
	float width = (monitorCount > 0) ? GetMonitorWidth(0) : 360.0;
	float height = (monitorCount > 0) ? GetMonitorHeight(0) : 200.0;
	int RefreshRate = (monitorCount > 0) ? GetMonitorRefreshRate(0) : 60;
	SetWindowSize(width / 2, height / 2);
	SetTargetFPS(RefreshRate);
	printf("width: %f, height: %f, refresh rate: %d, mointor count: %d\n", width, height, RefreshRate,
	       monitorCount);
	bool drawdev = false;
	bool connected = false;
	std::vector<samples::sampleStream *> *sbuff = new std::vector<samples::sampleStream *>;
	samples::sampleStream *sstream = new samples::sampleStream;
	sbuff->push_back(sstream);
	while(!WindowShouldClose()) { // Detect window close button or ESC key
		int len = state->addrRoot->lenth();
		char status[128] = {0};
		snprintf(status, 128, "found %d dev\n", len);
		BeginDrawing();
		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
		GuiLabel((Rectangle){0, 0, width, height}, status);
		if(drawdev) {
			//		drawDevice((Rectangle){0, 0, width, height},
			//(devices::device*)&state->devices->list.front());
		}
		if(len > 0) {
			if(!connected) {
				if(GuiButton((Rectangle){0, 0, 100, 100}, "connect")) {
					struct esp::scopeConf conf = {
					    .channels = 1,
					    .sampleRate = 20000,
					    .duration = 50,
					};
					devices::device *dev =
					    new devices::device(conf, state->addrRoot, &state->addrRoot->next->addr,
								sizeof(struct sockaddr_in));
					state->devices->list.push_back(dev);
					connected = true;
				};
			} else {
				devices::device * dev= state->devices->list.front();
				std::cout << "pringting\n";
				dev->readSamples(sbuff);
				printf("volt: %f\n", sstream->back().voltage);
	/*			while(!sstream->empty()) {
					sstream->pop();
				}*/
			}
		}
		EndDrawing();
	};
	CloseWindow(); // Close window and OpenGL context
	return 0;
}
