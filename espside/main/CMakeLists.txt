idf_component_register(
	SRCS "main.c"
	INCLUDE_DIRS "."
	REQUIRES esp_netif
	REQUIRES hal
	REQUIRES nvs_flash
	REQUIRES esp_wifi
	REQUIRES broadcaster
	REQUIRES btconfig
	REQUIRES esp_adc
	PRIV_REQUIRES driver esp_driver_gpio efuse esp_pm esp_ringbuf esp_mm
)
