#include "devices.h"
#include "helpertypes.h"
#include <recorder.h>
#include <addrlist.h>
#include <iostream>
#include <list>
#include <mainTypes.h>
#include <netinet/in.h>
#include <pthread.h>
#include <raygui.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <drawDevices.h>
#include <vector>
#include <drawGraph.h>

int main(void) {
	state *Mstate = new state;
	Mstate->devices = new helper::thwraper<std::list<devices::device *>>();
	InitWindow(0, 0, "teszt");
	int monitorCount = GetMonitorCount();
	float width = (monitorCount > 0) ? GetMonitorWidth(0) : 360.0;
	float height = (monitorCount > 0) ? GetMonitorHeight(0) : 200.0;
	int RefreshRate = (monitorCount > 0) ? GetMonitorRefreshRate(0) : 60;
	SetWindowSize(width / 2, height / 2);
	SetTargetFPS(RefreshRate);
	printf("width: %f, height: %f, refresh rate: %d, mointor count: %d\n", width, height, RefreshRate,
	       monitorCount);
	bool drawdev = false;
	bool connected = false;
	int samplelast = 0;
	int lastdelta = 0;
	int fcount = 0;

	while(!WindowShouldClose()) { // Detect window close button or ESC key
		int len = Mstate->addrRoot.lenth();
		char status[128] = {0};
		snprintf(status, 128, "found %d dev\n", len);
		BeginDrawing();
		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
		GuiLabel((Rectangle){100, 0, 100, 100}, status);
		if(drawdev) {
			//		drawDevice((Rectangle){0, 0, width, height},
			//(devices::device*)&state->devices->list.front());
		}
		if(len > 0) {
			if(!connected) {
				if(GuiButton((Rectangle){0, 0, 100, 100}, "connect")) {
					struct esp::scopeConf conf = {
					    .channels = 1,
					    .sampleRate = 40000,
					    .duration = 80,
					};
					devices::device *dev =
					    new devices::device(conf, &Mstate->addrRoot, &Mstate->addrRoot.next->addr,
								sizeof(struct sockaddr_in));
					Mstate->devices->wrlock();
					Mstate->devices->_data.push_back(dev);
					Mstate->devices->unlock();
					Mstate->devices->rdlock();
					for (int i = 0; i < conf.channels; i++) {
						record::recorder* rec = new record::recorder(new record::edgetriger(1.0, record::RISEING), new record::edgetriger(1.0, record::FALEING), Mstate->devices->_data.back()->buffer[i] ,&Mstate->recordstate.state, (size_t)3200);
						std::vector<record::recorder*> vec = {rec};
						Mstate->recordstate.recorders.push_back(vec);
					}
					Mstate->devices->unlock();
					connected = true;
				};
			}
		}
		Texture2D graph;
		if (!Mstate->recordstate.recorders.empty()){
			auto front = Mstate->recordstate.recorders.front();
			auto records = front[0]->getRecords();
			std::string text = "";
			text += "recorded: " + std::to_string(records.size()) + "\n";
			text += "last value: "  + std::to_string( ((records.size() != 0)? records.back().voltage : -1.0)) + "\n";
			if (fcount >= RefreshRate) {
				fcount = 0;
				auto recnow = records.size();
				lastdelta = recnow - samplelast;
				samplelast = recnow;
			}
			text += "sample rate: "  + std::to_string(lastdelta) + "\n";
			GuiLabel((Rectangle){200, 0, 200,100}, text.data());
			std::vector<std::vector<samples::sample>> graphs;
			graphs.push_back(records);
			std::vector<int> offsets = {0};
			graph = drawgraph(graphs, offsets, 600, 300, 3.3, 0, records.size() >600 ? records.size() -600: 0, records.size() >600 ? records.size(): 600);
			DrawTexture(graph, 0, 100, YELLOW);


		}else{
		GuiLabel((Rectangle){200, 0, 100,100}, "asd");
		};


		EndDrawing();
		UnloadTexture(graph);
		printf("mainok\n");
		fcount++;
	};
	CloseWindow(); // Close window and OpenGL context
	std::ofstream plot("plot.txt");
	auto recs = Mstate->recordstate.recorders.front()[0]->getRecords(); 
	plot << "array A[" << recs.size() +1  << "] = [ ";
	for (samples::sample &i : recs) {
		plot << i.voltage << ", ";	
	}
	plot << "0 ]\n ";	
	plot << "plot A with points title \"Array A\"\n";
	plot << "pause -1\n";
	plot.close();
	return 0;
}
