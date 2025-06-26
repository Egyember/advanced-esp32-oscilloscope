#include <chrono>
#include <cstdio>
#include <iostream>
#include <math.h>
#include <raylib.h>
#include <samples.h>
#include <vector>

#define cpuimpl

Texture2D drawgraph(std::vector<std::vector<samples::sample>> data, std::vector<Color> color,
		    unsigned int x, unsigned int y, float Vmax, float Vmin, unsigned int startTime,
		    unsigned int stopTime) {
#ifdef cpuimpl
	Image image = GenImageColor(x, y, BLACK);
	float voltconv = (Vmax - Vmin) / (float)y;
	float timeconv = (stopTime - startTime) / (float)x;
	for(int i = 0; i < data.size() && i < color.size(); i++) {
		auto sampl = data[i];
		for(int j = startTime; j < stopTime && j < (sampl.size()); j++) {
			int ycord = (int)std::floor(sampl[j].voltage / voltconv);
			int xcord = (j - startTime) / timeconv;
			ImageDrawPixel(&image, xcord, y - ycord, color[i]);
		}
	}
	Texture2D texture = LoadTextureFromImage(image);
	UnloadImage(image);
	return texture;
#endif
};
