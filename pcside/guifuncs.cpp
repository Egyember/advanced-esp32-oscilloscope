#include <mainTypes.h>
#include <raygui.h>
#include <raylib.h>
#include <arpa/inet.h>
#include <cstring>
void addDialog(state *Mstate){
	Mstate->gui.add.addDialog = !GuiWindowBox(Mstate->gui.popupBounds, "add");
	std::string addressed = "";
	Mstate->addrRoot.nodes.rdlock();
	for (auto n : Mstate->addrRoot.nodes._data) {
		char buff[16] = {0};
		inet_ntop(AF_INET, &((struct sockaddr_in *) &n.addr)->sin_addr, buff, sizeof(buff));
		addressed += buff;
		addressed += ";";
	}
	addressed = addressed.substr(0, addressed.size()-1);
	GuiListView(Mstate->gui.popupScroll, addressed.c_str(), &Mstate->gui.add.addIndex, &Mstate->gui.add.addActive);
	if (Mstate->gui.add.addActive != -1) {
		std::list<addrlist::addrllnode>::iterator it = Mstate->addrRoot.nodes._data.begin();
		std::advance(it, Mstate->gui.add.addActive);
		memcpy(&Mstate->gui.add.selectedAddress, &it->addr, sizeof(Mstate->gui.add.selectedAddress));
		Mstate->gui.add.setupDialog = true;
		Mstate->gui.add.addDialog = false;
	}
	Mstate->addrRoot.nodes.unlock();

};

void setupDialog(state *Mstate){
			Mstate->gui.add.setupDialog = !GuiWindowBox(Mstate->gui.popupBounds, "setup");
			Rectangle chanBound = Mstate->gui.popupScroll;
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
			if(GuiTextBox(chanInput, Mstate->gui.add.setup.chan, 32, Mstate->gui.add.setup.chanEdit)){
				Mstate->gui.add.setup.chanEdit = !Mstate->gui.add.setup.chanEdit;
			};
			GuiLabel(sampText, " sample rate:");
			if(GuiTextBox(sampInput, Mstate->gui.add.setup.samp, 32, Mstate->gui.add.setup.sampEdit)){
				Mstate->gui.add.setup.sampEdit = !Mstate->gui.add.setup.sampEdit;
			};
			GuiLabel(duraText, " duration:");
			if(GuiTextBox(duraInput, Mstate->gui.add.setup.duration, 32, Mstate->gui.add.setup.duraEdit)){
				Mstate->gui.add.setup.duraEdit = !Mstate->gui.add.setup.duraEdit;
			};

			Rectangle sendBound = duraBound;
			sendBound.y += sendBound.height;

			if (GuiButton(sendBound, "send")){
				std::string error = "";
				esp::scopeConf conf;
				std::vector<int> of;
				unsigned int chanParsed = -1;	 
				unsigned int sampParsed = -1;	 
				unsigned int duraParsed = -1;	 
				std::vector<record::recorder *> vec;
				if(sscanf(Mstate->gui.add.setup.chan, "%ud",&chanParsed) != 1){
					error = "invalid\n";
					goto error;
				}; 
				if (!(chanParsed > 0 && UINT8_MAX >= chanParsed)) {
					error = "invalid\n";
					goto error;
				}
				conf.channels = (uint8_t) chanParsed;
				if(sscanf(Mstate->gui.add.setup.samp, "%ud",&sampParsed) != 1){
					error = "invalid\n";
					goto error;
				}; 
				if (!(sampParsed > 0 && UINT32_MAX >= sampParsed)) {
					error = "invalid\n";
					goto error;
				}
				conf.sampleRate = (uint32_t) sampParsed;
				if(sscanf(Mstate->gui.add.setup.duration, "%ud",&duraParsed) != 1){
					error = "invalid\n";
					goto error;
				}; 
				if (!(duraParsed > 0 && UINT32_MAX >= duraParsed)) {
					error = "invalid\n";
					goto error;
				}
				conf.duration = (uint32_t) duraParsed;

				devices::device *dev;
				try{
				dev = new devices::device(conf, &Mstate->addrRoot, &Mstate->gui.add.selectedAddress,
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
				for(auto i: Mstate->devices->_data.back()->buffer) {
					record::recorder *rec = new record::recorder(
							new record::nevertriger(),
							new record::nevertriger(),
							i,
							&Mstate->recordstate.state, (size_t)conf.sampleRate*conf.duration, conf.sampleRate);
					vec.push_back(rec);
				}
				Mstate->recordstate.recorders.push_back(vec);
				Mstate->devices->unlock();
				Mstate->offsets.wrlock();
				for (int i = 0; i < conf.channels; i++) {
					of.push_back(0);
				}
				Mstate->offsets._data.push_back(of);
				Mstate->offsets.unlock();

error:
				Mstate->gui.add.setupDialog = false;
				if (error != "") {
					printf("error: %s\n", error.c_str());
				}
			};
};
