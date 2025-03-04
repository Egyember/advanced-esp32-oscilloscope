#include <addrlist.h>
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
	addrllroot *addrRoot = addrll_init();
	pthread_t scanner;
	pthread_create(&scanner, NULL, (void *(*)(void *))scanForEsp, addrRoot);
	InitWindow(300, 300, "teszt");
	SetTargetFPS(60);
	bool exitWindow = false;
	while (!exitWindow && !WindowShouldClose()){    // Detect window close button or ESC key 
	};
	CloseWindow();        // Close window and OpenGL context
	return 0;
}
