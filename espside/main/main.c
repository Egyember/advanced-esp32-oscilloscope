#include "broadcaster.h"
#include "btconfig.h"
#include "cc.h"
#include "esp_adc/adc_continuous.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "hal/adc_types.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>


#include <soc/soc_caps.h>


#define WIFIDONEBIT BIT0
#define WIFIFAILED BIT1
#define COMPORT 40001

const static uint8_t channelConfig[] = {ADC_CHANNEL_0, ADC_CHANNEL_3,ADC_CHANNEL_4,ADC_CHANNEL_5};
struct  scopeConf {
	uint8_t channels;
	uint32_t sampleRate; // expected ADC sampling frequency in Hz.
	uint32_t duration;   // in ms
};
#define CONFREADBUFFER (sizeof(uint32_t)*2 + sizeof(uint8_t))

static const char *MAIN_TAG = "MAIN";
static const char *WIFI_TAG = "MAIN/WIFI";
EventGroupHandle_t wifi_eventGroup;
struct localIp {
	uint32_t ip;
	uint32_t mask;
	SemaphoreHandle_t lock;
	TaskHandle_t broadcaster;
};
/*
 *used to get first avalable connection slot index
 -1 if no connection avalable
 */
struct localIp *localIp;

int retry = 0;
void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	if(event_base == WIFI_EVENT) {
		ESP_LOGI(WIFI_TAG, "wifi event");
		switch(event_id) {
		case WIFI_EVENT_STA_START:
			esp_wifi_connect();
			break;
		case WIFI_EVENT_STA_CONNECTED:
			ESP_LOGI(WIFI_TAG, "connected to wifi");
			esp_err_t err = esp_netif_dhcpc_start(arg);
			if(err == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED) {
				ESP_LOGI(WIFI_TAG, "dhcp aready running");
			};
			break;
		case WIFI_EVENT_STA_DISCONNECTED:
			ESP_LOGI(WIFI_TAG, "disconected trying to reconnect");
			retry++;
			if(retry > 3) {
				xEventGroupSetBits(wifi_eventGroup, WIFIFAILED);
				break;
			};
			esp_wifi_connect();
			break;
		default:
			ESP_LOGI(WIFI_TAG, "unhandelered event");
		}
	}
};

void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	if(event_base == IP_EVENT) {
		ESP_LOGI(WIFI_TAG, "ip event");
		if(event_id == IP_EVENT_STA_GOT_IP) {
			// ESP_ERROR_CHECK(esp_netif_dhcpc_stop(arg));
			ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
			ESP_LOGI(WIFI_TAG, "static ip:" IPSTR, IP2STR(&event->ip_info.ip));
			xSemaphoreTake(localIp->lock, portMAX_DELAY);
			localIp->ip = event->ip_info.ip.addr;
			localIp->mask = event->ip_info.netmask.addr;
			xTaskCreate(broadcaster, "broadcaster", 1024 * 2, localIp, tskIDLE_PRIORITY,
				    &(localIp->broadcaster));
			xSemaphoreGive(localIp->lock);
			xEventGroupSetBits(wifi_eventGroup, WIFIDONEBIT);
			retry = 0;

		} else if(event_id == IP_EVENT_STA_LOST_IP) {
			ESP_LOGI(WIFI_TAG, "lost ip address");
			xSemaphoreTake(localIp->lock, portMAX_DELAY);
			vTaskDelete(localIp->broadcaster);
			xSemaphoreGive(localIp->lock);
		};
	}
};

int readConfig(int soc, struct scopeConf *config){
	int avalable = -1;
	do {
		if (avalable != -1) {
			vTaskDelay(100 / portTICK_PERIOD_MS);
		};
		int err = lwip_ioctl(soc, FIONREAD, &avalable);
		if(err < 0) {
			ESP_LOGE(MAIN_TAG, "ioctl failed errno: %s", strerror(errno));
			return -1;
		};
		ESP_LOGI(MAIN_TAG, "%d byte avalable %d needed", avalable, CONFREADBUFFER);
	}while (avalable < CONFREADBUFFER);
	uint8_t buffer[CONFREADBUFFER];
	int err = read(soc, &buffer, CONFREADBUFFER);
	if(err < 0) {
		ESP_LOGE(MAIN_TAG, "read failed");
		return -1;
	};
	config->channels = buffer[0];
	config->duration = ntohl(*((uint32_t *)(&buffer[1]))); 
	config->sampleRate = ntohl(*((uint32_t *)(&buffer[5]))); 
	return 0;

};

void app_main(void) {
	ESP_ERROR_CHECK(nvs_flash_init());

	ESP_LOGI(MAIN_TAG, "seting up wifi");
	wifi_eventGroup = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(esp_netif_init());
	esp_netif_t *netint = esp_netif_create_default_wifi_sta();
	if(netint == NULL) {
		ESP_LOGE(MAIN_TAG, "netif == NULL");
		ESP_LOGE(MAIN_TAG, "jumping to cleanup");
		goto CLEANUP;
	}
	wifi_init_config_t wifiInitconf = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifiInitconf));
	esp_event_handler_instance_t wifieventh;
	esp_event_handler_instance_t ipeventh;
	localIp = malloc(sizeof *localIp);
	localIp->lock = xSemaphoreCreateMutex();

	ESP_ERROR_CHECK(
	    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, netint, &wifieventh));
	ESP_ERROR_CHECK(
	    esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, ip_event_handler, netint, &ipeventh));

	wifi_config_t wificonfig = {0};
	// reading config from nvs
	nvs_handle_t nvsHandle;
	{
		nvs_open("wifi_auth", NVS_READONLY, &nvsHandle);
		size_t wifiLen;
		nvs_get_str(nvsHandle, "ssid", NULL, &wifiLen);
		if(wifiLen > 32) {
			ESP_LOGE(MAIN_TAG, "too long ssid stored in nvs. Jumping to bt setup");
			btconfig(); // todo: implemnet
			esp_restart();
		};
		nvs_get_str(nvsHandle, "ssid", (char *)&wificonfig.sta.ssid, &wifiLen);

		nvs_get_str(nvsHandle, "passwd", NULL, &wifiLen);
		if(wifiLen > 64) { // implemention limit to passwd lenght is 64
			ESP_LOGE(MAIN_TAG, "too long ssid stored in nvs. Jumping to bt setup");
			btconfig(); // todo: implemnet
			esp_restart();
		};
		nvs_get_str(nvsHandle, "passwd", (char *)&wificonfig.sta.password, &wifiLen);

		uint8_t auth;
		nvs_get_u8(nvsHandle, "authmetod", (uint8_t *)&auth);
		wificonfig.sta.threshold.authmode = auth;
		nvs_get_u8(nvsHandle, "channel", &wificonfig.sta.channel);
	};
	ESP_LOGI(MAIN_TAG, "ssid: %s, password:%s, channel: %d", wificonfig.sta.ssid, wificonfig.sta.password,
		 wificonfig.sta.channel);

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wificonfig));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_LOGI(MAIN_TAG, "wifi_init_sta finished.");
	ESP_LOGI(MAIN_TAG, "event magic started");
	EventBits_t bits = xEventGroupWaitBits(wifi_eventGroup, WIFIDONEBIT | WIFIFAILED, pdTRUE, pdFALSE,
					       portMAX_DELAY); // todo: add fail bit and check
	if(bits & WIFIFAILED) {
		ESP_LOGW(MAIN_TAG,"WIFIDONEBIT: %d", bits & WIFIDONEBIT ? 1:0);
		ESP_LOGI(MAIN_TAG, "event magic failed entering config mode");
		btconfig();
		ESP_LOGI(MAIN_TAG, "reboot");
		esp_restart();
	};

	ESP_LOGI(MAIN_TAG, "wifi done");

	// starting tcp server
	int soc = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr = {.sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(COMPORT)};
	int err = bind(soc, (struct sockaddr *)&addr, sizeof(addr));
	if(err < 0) {
		ESP_LOGE(MAIN_TAG, "failed to bind soc jumping to cleanup");
		close(soc);
		goto CLEANUPWITHSOC;
	}
	err = listen(soc, 1);
	if(err < 0) {
		ESP_LOGE(MAIN_TAG, "failed to lisen soc jumping to cleanup");
		close(soc);
		goto CLEANUPWITHSOC;
	}
	ESP_LOGI(MAIN_TAG, "server whaiting for connection");
	while(true) {
		struct sockaddr_in inaddr;
		socklen_t inlen = sizeof(inaddr);
		int fd = accept(soc, (struct sockaddr *)&inaddr, &inlen);
		ESP_LOGI(MAIN_TAG, "got connection");
		struct scopeConf config;
		if(readConfig(fd, &config) < 0) {
			close(fd);
			ESP_LOGW(MAIN_TAG, "reading in confing failed");
			continue;
		};
		
		ESP_LOGI(MAIN_TAG, "config: \n\tchan: %d\n\tduration: %lu\n\tfreq: %lu\n", config.channels, config.duration, config.sampleRate);
		if (config.sampleRate < SOC_ADC_SAMPLE_FREQ_THRES_LOW) {
			ESP_LOGE(MAIN_TAG, "frequency must be greather then %dkHz", SOC_ADC_SAMPLE_FREQ_THRES_LOW/1000);
			close(fd);
			ESP_LOGW(MAIN_TAG, "reading in confing failed");
			continue;
		}
		if (config.sampleRate > SOC_ADC_SAMPLE_FREQ_THRES_HIGH) {
			ESP_LOGE(MAIN_TAG, "frequency must be less then %dkHz", SOC_ADC_SAMPLE_FREQ_THRES_HIGH/1000);
			close(fd);
			ESP_LOGW(MAIN_TAG, "reading in confing failed");
			continue;
		}
		uint32_t frameSize =
		    (uint32_t)floor((float)config.sampleRate * ((float)config.duration / 1000.0) * (float)SOC_ADC_DIGI_RESULT_BYTES); // 2*8= 16 bit
		ESP_LOGI(MAIN_TAG, "frameSize: %lu", frameSize);

		adc_continuous_handle_cfg_t adcConfigHandler = {
		    .conv_frame_size = frameSize,
		    .max_store_buf_size = frameSize * 4,
		};
		adc_continuous_handle_t adcHandler = NULL;
		ESP_ERROR_CHECK(adc_continuous_new_handle(&adcConfigHandler, &adcHandler));
		
		adc_digi_pattern_config_t *adcPatterns = malloc(sizeof(adc_digi_pattern_config_t) * config.channels);
		for (int i = 0; i<config.channels; i++) {
			adcPatterns[i].atten = ADC_ATTEN_DB_12; //150 mV ~ 2450 mV
			adcPatterns[i].channel = channelConfig[i];
			adcPatterns[i].unit = ADC_UNIT_1;        ///< SAR ADC 1
			adcPatterns[i].bit_width = ADC_BITWIDTH_12; //max selected by default
		};

		adc_continuous_config_t adcConfig = {
			.pattern_num = config.channels,
			.adc_pattern = adcPatterns,
			.sample_freq_hz = config.sampleRate,
			.format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
			.conv_mode = ADC_CONV_SINGLE_UNIT_1
		};
		ESP_LOGI(MAIN_TAG, "buffers ready");
		ESP_ERROR_CHECK(adc_continuous_config(adcHandler, &adcConfig));
		adc_continuous_start(adcHandler);
		ESP_LOGI(MAIN_TAG, "adc running");
		uint8_t *readbuffer = malloc(sizeof(uint8_t)*frameSize); //1448 is the max tcp data segment size
		uint32_t readData;
		ESP_LOGI(MAIN_TAG, "sending data");
		do{
			adc_continuous_read(adcHandler, readbuffer, sizeof(readbuffer), &readData, config.duration);
		}while(write(fd, readbuffer, readData)>0);
		free(readbuffer);
		adc_continuous_stop(adcHandler);
		close(fd);
		ESP_ERROR_CHECK(adc_continuous_deinit(adcHandler));
		free(adcPatterns);
	};
CLEANUPWITHSOC:
	close(soc);
CLEANUP:
	esp_wifi_stop();
	esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifieventh);
	esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, ipeventh);
	esp_wifi_deinit();
	esp_netif_destroy_default_wifi(netint);
	esp_netif_deinit();
	esp_event_loop_delete_default();
	nvs_flash_deinit();
}
