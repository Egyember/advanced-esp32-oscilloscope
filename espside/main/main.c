#include "broadcaster.h"
#include "btconfig.h"


#include "esp_adc/adc_continuous.h"

#include "esp_adc/../../adc_continuous_internal.h"

#include "hal/adc_ll.h"


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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>


#include <soc/soc_caps.h>
#include <soc/adc_periph.h>
#include <soc/adc_channel.h>
#include <hal/adc_ll.h>
#include <driver/i2s.h>
#include <soc/apb_ctrl_reg.h>

#define WIFIDONEBIT BIT0
#define WIFIFAILED BIT1
#define COMPORT 40001

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

//adc woodoo
// copied from componentes/driver/deprecated/adc_i2s_deprecated.c
#include "sdkconfig.h"
#include "esp_types.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "driver/rtc_io.h"
#include "freertos/FreeRTOS.h"
#include "hal/adc_ll.h"
#include "hal/adc_types.h"
extern portMUX_TYPE rtc_spinlock; //TODO: Will be placed in the appropriate position after the rtc module is finished.
#define ADC_ENTER_CRITICAL()  portENTER_CRITICAL(&rtc_spinlock)
#define ADC_EXIT_CRITICAL()  portEXIT_CRITICAL(&rtc_spinlock)


typedef struct {
    union {
        struct {
            uint8_t atten:     2;   /*!< ADC sampling voltage attenuation configuration. Modification of attenuation affects the range of measurements.
                                         0: measurement range 0 - 800mV,
                                         1: measurement range 0 - 1100mV,
                                         2: measurement range 0 - 1350mV,
                                         3: measurement range 0 - 2600mV. */
            uint8_t bit_width: 2;   /*!< ADC resolution.
-                                         0: 9 bit;
-                                         1: 10 bit;
-                                         2: 11 bit;
-                                         3: 12 bit. */
            int8_t channel:   4;   /*!< ADC channel index. */
        };
        uint8_t val;                /*!<Raw data value */
    };
} adc_digi_pattern_table_t;


typedef enum {
    ADC_DIGI_FORMAT_12BIT,  /*!<ADC to DMA data format,                [15:12]-channel, [11: 0]-12 bits ADC data (`adc_digi_output_data_t`). Note: For single convert mode. */
    ADC_DIGI_FORMAT_11BIT,  /*!<ADC to DMA data format, [15]-adc unit, [14:11]-channel, [10: 0]-11 bits ADC data (`adc_digi_output_data_t`). Note: For multi or alter convert mode. */
    ADC_DIGI_FORMAT_MAX,
} adc_digi_format_t;
typedef struct {
    uint32_t adc1_pattern_len;               /*!<Pattern table length for digital controller. Range: 0 ~ 16 (0: Don't change the pattern table setting).
                                                 The pattern table that defines the conversion rules for each SAR ADC. Each table has 16 items, in which channel selection,
                                                 resolution and attenuation are stored. When the conversion is started, the controller reads conversion rules from the
                                                 pattern table one by one. For each controller the scan sequence has at most 16 different rules before repeating itself. */
    uint32_t adc2_pattern_len;               /*!<Refer to ``adc1_pattern_len`` */
    adc_digi_pattern_table_t *adc1_pattern;  /*!<Pointer to pattern table for digital controller. The table size defined by `adc1_pattern_len`. */
    adc_digi_pattern_table_t *adc2_pattern;  /*!<Refer to `adc1_pattern` */
    adc_digi_convert_mode_t conv_mode;       /*!<ADC conversion mode for digital controller. See ``adc_digi_convert_mode_t``. */
    adc_digi_format_t format;                /*!<ADC output data format for digital controller. See ``adc_digi_format_t``. */
} adc_digi_config_t;

#define DIG_ADC_OUTPUT_FORMAT_DEFUALT (ADC_DIGI_FORMAT_12BIT)
#define DIG_ADC_ATTEN_DEFUALT         (ADC_ATTEN_DB_12)
#define DIG_ADC_BIT_WIDTH_DEFUALT     (3)   //3 for ADC_WIDTH_BIT_12
extern esp_err_t adc_common_gpio_init(adc_unit_t adc_unit, adc_channel_t channel);

static inline void adc_ll_digi_prepare_pattern_table(adc_unit_t adc_n, uint32_t pattern_index, adc_digi_pattern_table_t pattern)
{
    uint32_t tab;
    uint8_t index = pattern_index / 4;
    uint8_t offset = (pattern_index % 4) * 8;
    if (adc_n == ADC_UNIT_1) {
        tab = SYSCON.saradc_sar1_patt_tab[index];   // Read old register value
        tab &= (~(0xFF000000 >> offset));           // clear old data
        tab |= ((uint32_t)pattern.val << 24) >> offset; // Fill in the new data
        SYSCON.saradc_sar1_patt_tab[index] = tab;   // Write back
    } else { // adc_n == ADC_UNIT_2
        tab = SYSCON.saradc_sar2_patt_tab[index];   // Read old register value
        tab &= (~(0xFF000000 >> offset));           // clear old data
        tab |= ((uint32_t)pattern.val << 24) >> offset; // Fill in the new data
        SYSCON.saradc_sar2_patt_tab[index] = tab;   // Write back
    }
}

static inline void adc_ll_digi_set_output_format(bool data_sel)
{
    SYSCON.saradc_ctrl.data_sar_sel = data_sel;
}

static void adc_digi_controller_reg_set(const adc_digi_config_t *cfg)
{
    /* On ESP32, only support ADC1 */
    switch (cfg->conv_mode) {
    case ADC_CONV_SINGLE_UNIT_1:
        adc_ll_digi_set_convert_mode(ADC_LL_DIGI_CONV_ONLY_ADC1);
        break;
    case ADC_CONV_SINGLE_UNIT_2:
        adc_ll_digi_set_convert_mode(ADC_LL_DIGI_CONV_ONLY_ADC2);
        break;
    case ADC_CONV_BOTH_UNIT:
        adc_ll_digi_set_convert_mode(ADC_LL_DIGI_CONV_BOTH_UNIT);
        break;
    case ADC_CONV_ALTER_UNIT:
        adc_ll_digi_set_convert_mode(ADC_LL_DIGI_CONV_ALTER_UNIT);
        break;
    default:
        abort();
    }

    if (cfg->conv_mode & ADC_CONV_SINGLE_UNIT_1) {
        adc_ll_set_controller(ADC_UNIT_1, ADC_LL_CTRL_DIG);
        if (cfg->adc1_pattern_len) {
            adc_ll_digi_clear_pattern_table(ADC_UNIT_1);
            adc_ll_digi_set_pattern_table_len(ADC_UNIT_1, cfg->adc1_pattern_len);
            for (uint32_t i = 0; i < cfg->adc1_pattern_len; i++) {
                adc_ll_digi_prepare_pattern_table(ADC_UNIT_1, i, cfg->adc1_pattern[i]);
            }
        }
    }
    if (cfg->conv_mode & ADC_CONV_SINGLE_UNIT_2) {
        adc_ll_set_controller(ADC_UNIT_2, ADC_LL_CTRL_DIG);
        if (cfg->adc2_pattern_len) {
            adc_ll_digi_clear_pattern_table(ADC_UNIT_2);
            adc_ll_digi_set_pattern_table_len(ADC_UNIT_2, cfg->adc2_pattern_len);
            for (uint32_t i = 0; i < cfg->adc2_pattern_len; i++) {
                adc_ll_digi_prepare_pattern_table(ADC_UNIT_2, i, cfg->adc2_pattern[i]);
            }
        }
    }
    adc_ll_digi_set_output_format(cfg->format);
    adc_ll_digi_convert_limit_enable(ADC_LL_DEFAULT_CONV_LIMIT_EN);
    adc_ll_digi_set_convert_limit_num(ADC_LL_DEFAULT_CONV_LIMIT_NUM);
    //adc_ll_digi_set_data_source(ADC_I2S_DATA_SRC_ADC); //where the fuck the macro com from?
}

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
			xTaskCreate(broadcaster, "broadcaster", 1024 * 2, localIp, tskIDLE_PRIORITY+1,
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

int hexdump(unsigned char *src, size_t len, unsigned int with){
	if (!src) {
		return -1;
	}
	unsigned int l = 0;
	for (size_t i = 0; i < len; i++) {
		printf("%02x", src[i]);
		if (++l == with) {
			printf("\n");
			l = 0;
		}
	}
	printf("\n");
	return 0;
};

struct patternTableEntry{
	uint8_t ch_sel: 4;
	uint8_t bit_width: 2;
	uint8_t atten: 2;
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

		i2s_config_t i2s_conf = {
			.mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN,
			.sample_rate = config.sampleRate,
			.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
			.channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
			.communication_format = I2S_COMM_FORMAT_I2S_MSB,
			.intr_alloc_flags = 0,
			.dma_buf_count = 2,
			.dma_buf_len = 1024,
			.use_apll = false
		};
		ESP_ERROR_CHECK(i2s_driver_install(I2S_NUM_0, &i2s_conf, 0, NULL));

		hexdump((unsigned char *)APB_CTRL_APB_SARADC_SAR1_PATT_TAB1_REG, sizeof(uint32_t), 8);
		ESP_ERROR_CHECK(i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_0));

		const static uint8_t channelConfig[] = {ADC_CHANNEL_0, ADC_CHANNEL_3,ADC_CHANNEL_4,ADC_CHANNEL_5};
		// ADC setting
		hexdump((unsigned char *)APB_CTRL_APB_SARADC_SAR1_PATT_TAB1_REG, sizeof(uint32_t), 8);
		/*
		volatile struct patternTableEntry *patterns = (struct patternTableEntry *)APB_CTRL_APB_SARADC_SAR1_PATT_TAB1_REG;
		for (int i = 0; i<4; i++) {
			patterns[i].atten = ADC_ATTEN_DB_12; //150 mV ~ 2450 mV
			patterns[i].ch_sel = channelConfig[i];
			patterns[i].bit_width = 0b11;//ADC_BITWIDTH_12; //max selected by default
		};
		SYSCON.saradc_ctrl.sar1_patt_len =  config.channels<4 ?config.channels -1 : 0;
		if (config.channels>4){
			ESP_LOGE(MAIN_TAG, "channels more than 4 not implemented");
		};
		*/

		adc_digi_pattern_table_t adc1_pattern[4];
		adc_digi_config_t dig_cfg = {
			.format = DIG_ADC_OUTPUT_FORMAT_DEFUALT,
			.conv_mode = ADC_CONV_SINGLE_UNIT_1,
		};

		for (int i = 0; i< 4; i++) {
			adc1_pattern[i].atten = DIG_ADC_ATTEN_DEFUALT;
			adc1_pattern[i].bit_width = DIG_ADC_BIT_WIDTH_DEFUALT;
			adc1_pattern[i].channel = channelConfig[i];
			adc_common_gpio_init(ADC_UNIT_1, channelConfig[i]);
		} 

		dig_cfg.adc1_pattern_len = config.channels <5 ? config.channels : 1;
		dig_cfg.adc1_pattern = adc1_pattern;
		ADC_ENTER_CRITICAL();
		adc_ll_digi_set_fsm_time(ADC_LL_FSM_RSTB_WAIT_DEFAULT, ADC_LL_FSM_START_WAIT_DEFAULT,
				ADC_LL_FSM_STANDBY_WAIT_DEFAULT);
		adc_ll_set_sample_cycle(ADC_LL_SAMPLE_CYCLE_DEFAULT);
		adc_ll_pwdet_set_cct(ADC_LL_PWDET_CCT_DEFAULT);
		adc_ll_digi_output_invert(ADC_UNIT_1, ADC_LL_DIGI_DATA_INVERT_DEFAULT(ADC_UNIT_1));
		adc_ll_digi_set_clk_div(ADC_LL_DIGI_SAR_CLK_DIV_DEFAULT);
		adc_digi_controller_reg_set(&dig_cfg);
		ADC_EXIT_CRITICAL();

		hexdump((unsigned char *)APB_CTRL_APB_SARADC_SAR1_PATT_TAB1_REG, sizeof(uint32_t), 8);
		ESP_ERROR_CHECK(i2s_adc_enable(I2S_NUM_0));

		// delay for I2S bug workaround
		vTaskDelay(10 / portTICK_PERIOD_MS);

		// ***IMPORTANT*** enable continuous adc sampling
		SYSCON.saradc_ctrl2.meas_num_limit = 0;


		ESP_LOGI(MAIN_TAG, "adc running");
		uint8_t *readbuffer = malloc(sizeof(uint8_t)*frameSize); //1448 is the max tcp data segment size
		for (int i = 0; i < sizeof(readbuffer); i++) {
			readbuffer[i] = -1;
		}
		memset(readbuffer, 0xff, sizeof(uint8_t)*frameSize);
		size_t readData;
		ESP_LOGI(MAIN_TAG, "sending data");
		hexdump((unsigned char *)APB_CTRL_APB_SARADC_SAR1_PATT_TAB1_REG, sizeof(uint32_t), 8);
		do{
			ESP_ERROR_CHECK(i2s_read(I2S_NUM_0, readbuffer, sizeof(uint8_t)*frameSize, &readData, portMAX_DELAY));
	//		ESP_LOGI(MAIN_TAG, "read %d bytes", readData);
		}while(write(fd, readbuffer, readData)>=0);
		ESP_LOGE(MAIN_TAG, "connection falied");
		free(readbuffer);
		close(fd);
		ESP_ERROR_CHECK(i2s_adc_disable(I2S_NUM_0));
		ESP_ERROR_CHECK(i2s_driver_uninstall(I2S_NUM_0));

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

};
