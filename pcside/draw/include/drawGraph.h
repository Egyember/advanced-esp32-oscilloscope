#ifndef DRAWGRAP
#define DRAWGRAP
#include <vector>
#include <samples.h>
#include <raylib.h>
Texture2D drawgraph(std::vector<std::vector<samples::sample>> data, std::vector<int> offsets ,unsigned int x,unsigned int y, float Vmax, float Vmin, unsigned int startTime, unsigned int stopTime);
#endif
