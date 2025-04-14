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
	Pstate state = malloc(sizeof(struct state));
	state->addrRoot = addrll_init();
	pthread_t scanner;
	pthread_create(&scanner, NULL, (void *(*)(void *))scanForEsp, state->addrRoot);
	InitWindow(0, 0, "teszt");
	int monitorCount = GetMonitorCount();
	int width = (monitorCount > 0) ? GetMonitorWidth(0) : 360;
	int height = (monitorCount > 0) ? GetMonitorHeight(0) : 200;
	int RefreshRate =(monitorCount > 0) ?  GetMonitorRefreshRate(0) : 60;
	SetWindowSize(width/2, height/2);
	SetTargetFPS(RefreshRate);
	printf("width: %d, height: %d, refresh rate: %d, mointor count: %d\n", width, height, RefreshRate, monitorCount);
	bool drawdev = false;
	while (!WindowShouldClose()){    // Detect window close button or ESC key 
		BeginDrawing();
			int len = addrll_lenth(state->addrRoot);
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
