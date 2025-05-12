#include "devices.h"
#include "helpertypes.h"
#include <pthread.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <addrlist.h>
#include <mainTypes.h>
#include <raygui.h>
#include <iostream>

#include <drawDevices.h>

int main(void) {
	Pstate state = (Pstate)malloc(sizeof(struct state));
	state->addrRoot = new addrlist::root;
	state->devices = new helper::thslist<devices::device*>;
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
	while(!WindowShouldClose()) { // Detect window close button or ESC key
		int len = state->addrRoot->lenth();
		char status[128] = {0};
		snprintf(status, 128, "found %d dev\n", len);
		BeginDrawing();
		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
		GuiLabel((Rectangle){0, 0, width, height}, status);
		if(drawdev) {
			drawDevice((Rectangle){0, 0, width, height}, (devices::device*)&state->devices->list.front());
		}
		if(len > 0) {
			if(GuiButton((Rectangle){0, 0, 100, 100}, "connect")) {
				struct scopeConf conf = {
					.channels = 1,
					.sampleRate = 40000,
					.duration = 100,
				};
				 devices::device* dev = new devices::device(conf, state->addrRoot, &state->addrRoot->next->addr, sizeof(struct sockaddr_in));
				state->devices->list.push_back(dev);
			}
		}
		EndDrawing();
	};
	CloseWindow(); // Close window and OpenGL context
	return 0;
}
