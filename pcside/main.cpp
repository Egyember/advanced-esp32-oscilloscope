#include "devices.h"
#include "espsiteTypes.h"
#include "helpertypes.h"
#include <addrlist.h>
#include <cstdlib>
#include <cstring>
#include <drawDevices.h>
#include <drawGraph.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <mainTypes.h>
#include <netinet/in.h>
#include <pthread.h>
#include <raygui.h>
#include <raylib.h>
#include <recorder.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>
#include <guihelper.h>
#include <helperMacros.h>

int main(void) {
	SetTraceLogLevel(LOG_ERROR);
	state *Mstate = new state;
	Mstate->devices = new helper::thwraper<std::list<devices::device *>>();
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(0, 0, "teszt");
	int monitorCount = GetMonitorCount();
	float width = (monitorCount > 0) ? GetMonitorWidth(0) : 360.0;
	float height = (monitorCount > 0) ? GetMonitorHeight(0) : 200.0;
	int RefreshRate = (monitorCount > 0) ? GetMonitorRefreshRate(0) : 60;
	SetWindowSize(width / 2, height / 2);
	SetTargetFPS(RefreshRate);
	printf("width: %f, height: %f, refresh rate: %d, mointor count: %d\n", width, height, RefreshRate,
	       monitorCount);
	int samplelast = 0;
	int lastdelta = 0;
	int fcount = 0;


	while(!WindowShouldClose()) { // Detect window close button or ESC key
		BeginDrawing();
		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
		width = (monitorCount > 0) ? GetScreenWidth() : 360.0;
		height = (monitorCount > 0) ? GetScreenHeight() : 200.0;

		Mstate->gui.popupBounds = {width * 0.3f, height * 0.3f, width * 0.4f, height * 0.4f};
		Mstate->gui.popupScroll = Mstate->gui.popupBounds;
		Mstate->gui.popupScroll.y += 23;
		Mstate->gui.popupScroll.height -= 23;

		deviderpoint deviderpoint = {(float)width * 0.85f, (float)height * (float)0.1};
		Rectangle buttons = {deviderpoint.array[0], 0, (float)width - deviderpoint.array[0], deviderpoint.array[1]};

		// button bounds
		Rectangle addButton = buttons;
		addButton.width /= 4;
		Rectangle saveButton = addButton;
		saveButton.x += saveButton.width;
		Rectangle clearButton = saveButton;
		clearButton.x += clearButton.width;
		Rectangle recordButton = clearButton;
		recordButton.x += recordButton.width;

		// add dialog button
		if(GuiButton(addButton, "+")) {
			Mstate->gui.add.addDialog = true;
			Mstate->gui.add.addActive = -1;
			Mstate->gui.add.addIndex = -1;
		};
		GuiButton(saveButton, "#2#");
		if(Mstate->recordstate.state == record::RECORED) {
			if (GuiButton(recordButton, "#132#")) {
				Mstate->recordstate.state = record::STOP;
			};
		} else {
			if (GuiButton(recordButton, "#131#")){
				Mstate->recordstate.state = record::RECORED;
			};
		}
		if(GuiButton(clearButton, "#143#")) {
			for (auto d : Mstate->recordstate.recorders) {
				for (auto r: d) {
					r->clear();
				}
			}
		};
		Texture2D graph;
		if (!Mstate->recordstate.recorders.empty()){
			Rectangle slider = {
				.x = 0,
				.y = height * 0.98f,
				.width = deviderpoint.cord.x,
				.height =  height * 0.02f,
			};
			auto front = Mstate->recordstate.recorders.front();
			auto recordlen = front[0]->buffersize();
			auto records = front[0]->getRecords(recordlen - 600 > 0 ? recordlen - 600 : 0, recordlen);
			std::string text = "";
			text += "recorded: " + std::to_string(recordlen) + "\n";
			text +=
				"last value: " + std::to_string(((recordlen != 0) ? records.back().voltage : -1.0)) + "\n";
			if(fcount >= RefreshRate) {
				fcount = 0;
				auto recnow = recordlen;
				lastdelta = recnow - samplelast;
				samplelast = recnow;
			}
			text += "sample rate: " + std::to_string(lastdelta) + "\n";
			GuiLabel((Rectangle){200, 0, 200, 100}, text.data());
			std::vector<std::vector<samples::sample>> data;
			data.push_back(records);
			std::vector<Color> colors;
			colors.push_back(WHITE);
			graph = drawgraph(data, colors, deviderpoint.array[0], height - deviderpoint.array[1] - slider.height, 3.3, 0,
					records.size() > 600 ? records.size() - 600 : 0,
					records.size() > 600 ? records.size() : 600);
			DrawTexture(graph, 0, deviderpoint.array[1], WHITE);
			GuiSlider(slider, NULL, NULL, &Mstate->gui.slider, 0, recordlen);

		};
		
		GuiLabel((Rectangle){400, 0, 100, 100}, std::to_string(GetFPS()).data());

		if(Mstate->gui.add.addDialog) {
			addDialog(Mstate);
		}
		if (Mstate->gui.add.setupDialog) {
			setupDialog(Mstate);
		}
		EndDrawing();
		UnloadTexture(graph);
		fcount++;

	};
	CloseWindow(); // Close window and OpenGL context
	std::ofstream plot("plot.txt");
	auto recs = Mstate->recordstate.recorders.front()[0]->getRecords();
	plot << "array A[" << recs.size() + 1 << "] = [ ";
	for(samples::sample &i : recs) {
		plot << i.voltage << ", ";
	}
	plot << "0 ]\n ";
	plot << "plot A with points title \"Array A\"\n";
	plot << "pause -1\n";
	plot.close();
	return 0;
}
