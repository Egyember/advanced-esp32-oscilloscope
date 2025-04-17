#include "devices.h"
#include <pthread.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <raygui.h>
#include <addrlist.h>
#include <mainTypes.h>

#include <drawDevices.h>

int main(void) {
	Pstate state = (Pstate)malloc(sizeof(struct state));
	state->addrRoot = new addrlist::root;
	InitWindow(0, 0, "teszt");
	int monitorCount = GetMonitorCount();
	float width = (monitorCount > 0) ? GetMonitorWidth(0) : 360.0;
	float height = (monitorCount > 0) ? GetMonitorHeight(0) : 200.0;
	int RefreshRate =(monitorCount > 0) ?  GetMonitorRefreshRate(0) : 60;
	SetWindowSize(width/2, height/2);
	SetTargetFPS(RefreshRate);
	printf("width: %f, height: %f, refresh rate: %d, mointor count: %d\n", width, height, RefreshRate, monitorCount);
	bool drawdev = false;
	while (!WindowShouldClose()){    // Detect window close button or ESC key 
		BeginDrawing();
			int len = state->addrRoot->lenth();
			char status[128] = {0};
			snprintf(status, 128,"found %d dev\n",len);
			ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
			GuiLabel((Rectangle){0,0, width, height}, status);
			if (drawdev) {
				drawDevice((Rectangle){0,0, width, height}, state->devices);
			}
		EndDrawing();
	};
	CloseWindow();        // Close window and OpenGL context
	return 0;
}
