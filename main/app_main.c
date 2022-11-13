#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_timer.h"

#include "driver/gpio.h"

#define SCAN_LIST_SIZE 20
#define INDICATOR_GPIO GPIO_NUM_13
#define SIGNAL_PERIOD 100000

wifi_ap_record_t apInfo[SCAN_LIST_SIZE];
uint16_t apCount = 0;

esp_timer_handle_t runSignalTimer;
esp_timer_handle_t stopSignalTimer;

static void wifiScan(void);
static void runSignal(void* arg);
static void stopSignal(void* arg);

void app_main(void)
{
    gpio_pad_select_gpio(INDICATOR_GPIO);
    gpio_set_direction(INDICATOR_GPIO, GPIO_MODE_OUTPUT);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    memset(apInfo, 0, sizeof(apInfo));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    const esp_timer_create_args_t runSignalConfig = {
            .callback = &runSignal,
            .name = "runSignal"
    };
    ESP_ERROR_CHECK(esp_timer_create(&runSignalConfig, &runSignalTimer));

    const esp_timer_create_args_t stopSignalConfig = {
            .callback = &stopSignal,
            .name = "stopSignal"
    };
    ESP_ERROR_CHECK(esp_timer_create(&stopSignalConfig, &stopSignalTimer));

    uint64_t delayPeriod;
    uint8_t clearTimer = 1;
    while (true) {
        wifiScan();
        clearTimer = 1;

        for (int i = 0; (i < SCAN_LIST_SIZE) && (i < apCount); i++) {
            if (apInfo[i].rssi > -50) {
                delayPeriod = ( -1 * apInfo[i].rssi - 34) * 5 * 10000;
                if (200000 > delayPeriod) {
                    delayPeriod = 200000;
                }

                clearTimer = 0;

                if (esp_timer_is_active(runSignalTimer)) {
                    ESP_ERROR_CHECK(esp_timer_stop(runSignalTimer));
                }

                ESP_ERROR_CHECK(esp_timer_start_periodic(runSignalTimer, delayPeriod));
            }
        }

        if (clearTimer != 0 && esp_timer_is_active(runSignalTimer)) {
            ESP_ERROR_CHECK(esp_timer_stop(runSignalTimer));
        }
    }
}

static void wifiScan(void)
{
    uint16_t number = SCAN_LIST_SIZE;
    esp_wifi_scan_start(NULL, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, apInfo));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&apCount));
}

static void runSignal(void* arg)
{
    gpio_set_level(INDICATOR_GPIO, 1);

    ESP_LOGI("delayPeriod", "runSignal");

    if (esp_timer_is_active(stopSignalTimer)) {
        ESP_ERROR_CHECK(esp_timer_stop(stopSignalTimer));
    }

    ESP_ERROR_CHECK(esp_timer_start_once(stopSignalTimer, SIGNAL_PERIOD));
}

static void stopSignal(void* arg)
{
    ESP_LOGI("delayPeriod", "stopSignal");
    gpio_set_level(INDICATOR_GPIO, 0);
}
