idf_component_register(
	SRCS "main.c"
	INCLUDE_DIRS "."
	REQUIRES esp_netif
	REQUIRES nvs_flash
	REQUIRES esp_wifi
	REQUIRES broadcaster
	REQUIRES btconfig
	REQUIRES esp_adc
)
