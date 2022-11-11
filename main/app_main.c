#include <string.h>
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

uint8_t TRIGGER_AP[33] = "TanukiTheGreat";
wifi_ap_record_t apInfo[SCAN_LIST_SIZE];
uint16_t apCount = 0;
uint8_t indicatorLevel = 1;

static void wifiScan(void);
static void triggerSignal(void* arg);

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

    const esp_timer_create_args_t anomalySignal = {
            .callback = &triggerSignal,
            .name = "triggerSignal"
    };

    esp_timer_handle_t anomalySignalTimer;
    ESP_ERROR_CHECK(esp_timer_create(&anomalySignal, &anomalySignalTimer));

    float coefficient;
    uint64_t delayPeriod;
    while (true) {
        wifiScan();

        for (int i = 0; (i < SCAN_LIST_SIZE) && (i < apCount); i++) {
            //if (apInfo[i].ssid == TRIGGER_AP) {
                if (apInfo[i].rssi > -50) {
                    coefficient = apInfo[i].rssi * 0.6;
                    delayPeriod = (coefficient * coefficient + 100) * 1000;

                    ESP_LOGI("qwe", "wifiScan %lld", delayPeriod);
                    if (esp_timer_is_active(anomalySignalTimer)) {
                        ESP_ERROR_CHECK(esp_timer_stop(anomalySignalTimer));
                    }

                    ESP_ERROR_CHECK(esp_timer_start_periodic(anomalySignalTimer, delayPeriod));
                }
            //}
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

static void triggerSignal(void* arg)
{
    ESP_LOGI("qwe", "triggerSignal");
    gpio_set_level(INDICATOR_GPIO, indicatorLevel);
    indicatorLevel = indicatorLevel > 0 ? 0 : 1;
}
