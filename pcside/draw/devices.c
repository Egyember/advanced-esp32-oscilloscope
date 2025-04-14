#include <assert.h>
#include <devices.h>
#include <raygui.h>
#include <arpa/inet.h>

void drawDevice(Rectangle bounds, struct device *dev){
	assert(dev);
	char ip[16] = {0};
	inet_ntop(AF_INET, dev->address, ip, sizeof(struct sockaddr_in));
	GuiLabel(bounds, ip);
	return;
}
