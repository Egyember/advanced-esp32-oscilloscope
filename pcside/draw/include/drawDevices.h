#ifndef drawDev
#define drawDev
#include <devices.h>
#include <raygui.h>
#include <vector>
namespace draw{
	void drawDeviceGraps(Rectangle bounds, devices::device *dev, std::vector<Color> colors);
}
#endif
