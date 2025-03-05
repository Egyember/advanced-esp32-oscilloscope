#include <espsiteTypes.h>
#include <devices.h>

struct device *devices_connect(struct scopeConf config);
int devices_append(devices devices, struct device dev);
int devices_disconnect(devices devices, struct device dev);
