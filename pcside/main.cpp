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
	bool connected = false;
	int samplelast = 0;
	int lastdelta = 0;
	int fcount = 0;

	bool addDialog = false;
	int addIndex = -1;
	int addActive = -1;
	struct sockaddr selectedAddress = {0};
	bool setupDialog = false;

	while(!WindowShouldClose()) { // Detect window close button or ESC key
		BeginDrawing();
		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
		width = (monitorCount > 0) ? GetScreenWidth() : 360.0;
		height = (monitorCount > 0) ? GetScreenHeight() : 200.0;

		Rectangle popupBounds = {width * 0.3f, height * 0.3f, width * 0.4f, height * 0.4f};
		Rectangle popupScroll = popupBounds;
		popupScroll.y += 23;
		popupScroll.height -= 23;

		float deviderpoint[2] = {(float)width * 0.9f, (float)height * (float)0.1};
		Rectangle buttons = {deviderpoint[0], 0, (float)width - deviderpoint[0], deviderpoint[1]};

		// button bounds
		Rectangle addButton = buttons;
		addButton.width /= 3;
		Rectangle saveButton = buttons;
		saveButton.width /= 3;
		saveButton.x += saveButton.width;
		Rectangle recordButton = buttons;
		recordButton.width /= 3;
		recordButton.x += recordButton.width * 2;

		// add dialog button
		if(GuiButton(addButton, "+")) {
			addDialog = true;
			addActive = -1;
			addIndex = -1;
		};
		if(addDialog) {
			addDialog = !GuiWindowBox(popupBounds, "add");
			std::string addressed = "";
			Mstate->addrRoot.nodes.rdlock();
			for (auto n : Mstate->addrRoot.nodes._data) {
				char buff[16] = {0};
				inet_ntop(AF_INET, &((struct sockaddr_in *) &n.addr)->sin_addr, buff, sizeof(buff));
				addressed += buff;
				addressed += ";";
			}
			addressed = addressed.substr(0, addressed.size()-1);
			GuiListView(popupScroll, addressed.c_str(), &addIndex, &addActive);
			if (addActive != -1) {
				std::list<addrlist::addrllnode>::iterator it = Mstate->addrRoot.nodes._data.begin();
				std::advance(it, addActive);
				memcpy(&selectedAddress, &it->addr, sizeof(selectedAddress));
				setupDialog = true;
				addDialog = false;
			}
			Mstate->addrRoot.nodes.unlock();
		}
		if (setupDialog) {
			setupDialog = !GuiWindowBox(popupBounds, "setup");
			static char chan[33]; //0 inicialized becouse static, +1 for null byte
			static char samp[33];
			static char duration[33];
			static bool chanEdit;
			static bool sampEdit;
			static bool duraEdit;
			Rectangle chanBound = popupScroll;
			chanBound.height /= 4;

			Rectangle chanText = chanBound;
			chanText.width /=2; 
			Rectangle chanInput = chanText;
			chanInput.x += chanText.width;

			Rectangle sampBound = chanBound;
			sampBound.y += chanBound.height;

			Rectangle sampText = sampBound;
			sampText.width /=2; 
			Rectangle sampInput = sampText;
			sampInput.x += chanText.width;

			Rectangle duraBound = sampBound;
			duraBound.y += chanBound.height;

			Rectangle duraText = duraBound;
			duraText.width /=2; 
			Rectangle duraInput = duraText;
			duraInput.x += duraText.width;

			GuiLabel(chanText, " number of channels:");
			if(GuiTextBox(chanInput, chan, 32, chanEdit)){
				chanEdit = !chanEdit;
			};
			GuiLabel(sampText, " sample rate:");
			if(GuiTextBox(sampInput, samp, 32, sampEdit)){
				sampEdit = !sampEdit;
			};
			GuiLabel(duraText, " duration:");
			if(GuiTextBox(duraInput, duration, 32, duraEdit)){
				duraEdit = !duraEdit;
			};

			Rectangle sendBound = duraBound;
			sendBound.y += sendBound.height;

			if (GuiButton(sendBound, "send")){
				std::string error = "";
				esp::scopeConf conf;
				unsigned int chanParsed = -1;	 
				unsigned int sampParsed = -1;	 
				unsigned int duraParsed = -1;	 
				std::vector<record::recorder *> vec;
				if(sscanf(chan, "%ud",&chanParsed) != 1){
					error = "invalid\n";
					goto error;
				}; 
				if (!(chanParsed > 0 && UINT8_MAX >= chanParsed)) {
					error = "invalid\n";
					goto error;
				}
				conf.channels = (uint8_t) chanParsed;
				if(sscanf(samp, "%ud",&sampParsed) != 1){
					error = "invalid\n";
					goto error;
				}; 
				if (!(sampParsed > 0 && UINT32_MAX >= sampParsed)) {
					error = "invalid\n";
					goto error;
				}
				conf.sampleRate = (uint32_t) sampParsed;
				if(sscanf(duration, "%ud",&duraParsed) != 1){
					error = "invalid\n";
					goto error;
				}; 
				if (!(duraParsed > 0 && UINT32_MAX >= duraParsed)) {
					error = "invalid\n";
					goto error;
				}
				conf.sampleRate = (uint32_t) duraParsed;

				devices::device *dev;
				try{
				dev = new devices::device(conf, &Mstate->addrRoot, &selectedAddress,
							sizeof(struct sockaddr_in));
				}
				catch (...){
					error = "constructor";
					goto error;
				};
				Mstate->devices->wrlock();
				Mstate->devices->_data.push_back(dev);
				Mstate->devices->unlock();

				Mstate->devices->rdlock();
				for(int i = 0; i < conf.channels; i++) {
					record::recorder *rec = new record::recorder(
							new record::edgetriger(1.0, record::RISEING),
							new record::edgetriger(1.0, record::FALEING),
							Mstate->devices->_data.back()->buffer[i],
							&Mstate->recordstate.state, (size_t)3200, 100000);
					vec.push_back(rec);
				}
				Mstate->recordstate.recorders.push_back(vec);
				Mstate->devices->unlock();

error:
				setupDialog = false;
				if (error != "") {
					printf("error: %s\n", error.c_str());
				}
#warning remove this after after testing
				connected = true;
			};

		}
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
		int len = Mstate->addrRoot.lenth();
		char status[128] = {0};
		snprintf(status, 128, "found %d dev\n", len);
		GuiLabel((Rectangle){100, 0, 100, 100}, status);
		if(len > 0) {
			if(!connected) {
				if(GuiButton((Rectangle){0, 0, 100, 100}, "connect")) {
					struct esp::scopeConf conf = {
						.channels = 1,
						.sampleRate = 100000,
						.duration = 80,
					};
					Mstate->addrRoot.nodes.wrlock();
					auto addr= Mstate->addrRoot.nodes._data.front().addr;
					Mstate->addrRoot.nodes.unlock();
					devices::device *dev =
						new devices::device(conf, &Mstate->addrRoot, &addr,
								sizeof(struct sockaddr_in));
					Mstate->devices->wrlock();
					Mstate->devices->_data.push_back(dev);
					Mstate->devices->unlock();
					Mstate->devices->rdlock();
					for(int i = 0; i < conf.channels; i++) {
						record::recorder *rec = new record::recorder(
								new record::edgetriger(1.0, record::RISEING),
								new record::edgetriger(1.0, record::FALEING),
								Mstate->devices->_data.back()->buffer[i],
								&Mstate->recordstate.state, (size_t)3200, 100000);
						std::vector<record::recorder *> vec = {rec};
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
			std::vector<std::vector<samples::sample>> graphs;
			graphs.push_back(records);
			std::vector<int> offsets = {0};
			graph = drawgraph(graphs, offsets, 600, 300, 3.3, 0,
					records.size() > 600 ? records.size() - 600 : 0,
					records.size() > 600 ? records.size() : 600);
			DrawTexture(graph, 0, 100, YELLOW);

		} else {
			GuiLabel((Rectangle){200, 0, 100, 100}, "asd");
		};
		GuiLabel((Rectangle){400, 0, 100, 100}, std::to_string(GetFPS()).data());

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
