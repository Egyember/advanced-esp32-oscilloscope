#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <raygui.h>
#include <addrlist.h>
#include <mainTypes.h>

const static char *search = "oscilloscope here";

void *scanForEsp(addrllroot *root) {
	int soc = socket(AF_INET, SOCK_DGRAM, 0);
	if(soc < 0) {
		printf("Unable to create socket errno: %d", errno);
	}
	struct sockaddr_in localAddr = {.sin_addr = INADDR_ANY, .sin_port = htons(40000), .sin_family = AF_INET};
	if(bind(soc, (const struct sockaddr *)&localAddr, sizeof(localAddr)) < 0) {
		printf("can't bind to socket\n");
		exit(-1);
	};
	char buffer[strlen(search)];
	memset(buffer, '\0', strlen(search));
	printf("init\n");
	while(true) {
		struct sockaddr addr;
		socklen_t addrlen = sizeof(addr);
		recvfrom(soc, &buffer, sizeof(buffer), 0, &addr, &addrlen);
		printf("foo\n");
		if(strncmp(buffer, search, strlen(search)) == 0) {
			addrll_update(root, addr);
			printf("asd\n");
		};
	}
	return NULL;
}

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
	while (!WindowShouldClose()){    // Detect window close button or ESC key 
		BeginDrawing();
			ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
		EndDrawing();
	};
	CloseWindow();        // Close window and OpenGL context
	return 0;
}
