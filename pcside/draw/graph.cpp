#include <vector>
#include <samples.h>
#include <raylib.h>
#include <math.h>

#define cpuimpl
Texture2D drawgraph(std::vector<std::vector<samples::sample>> data, std::vector<int> offsets ,unsigned int x,unsigned int y, float Vmax, float Vmin, unsigned int startTime, unsigned int stopTime){
#ifdef cpuimpl
	Image image = GenImageColor(x, y, BLACK);
	float voltconv = (Vmax - Vmin) / (float) y;
	float timeconv = (stopTime -startTime) / (float) x;
	for (auto sampl : data) {
		for (int i = startTime; i < stopTime && i < sampl.size(); i++) {
			int ycord = (int)std::floor( sampl[i].voltage / voltconv);
			int xcord = (i - startTime) / timeconv;
			ImageDrawPixel(&image, xcord, y - ycord, WHITE);
		}
	}
	
	Texture2D texture = LoadTextureFromImage(image);
	UnloadImage(image);
	return texture;
#endif
};
