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
wifi_ap_record_t ap_info[SCAN_LIST_SIZE];
uint16_t apCount = 0;
uint8_t indicatorLevel = 1;

static void wifiScan(void);
static void triggerSignal();

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    gpio_pad_select_gpio(INDICATOR_GPIO);
    gpio_set_direction(INDICATOR_GPIO, GPIO_MODE_OUTPUT);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

//    const esp_timer_create_args_t anomalySignal = {
//            .callback = &triggerSignal,
//            .name = "triggerSignal"
//    };
//
//    esp_timer_handle_t anomalySignalTimer;
//    ESP_ERROR_CHECK(esp_timer_create(&anomalySignal, &anomalySignalTimer));

    //float coefficient;
    //uint64_t intensity;
    while (true) {
        wifiScan();
        triggerSignal();
        vTaskDelay(1000/portTICK_PERIOD_MS);
        for (int i = 0; (i < SCAN_LIST_SIZE) && (i < apCount); i++) {
            //if (ap_info[i].ssid == TRIGGER_AP) {
                //if (ap_info[i].rssi > -50) {
                   // coefficient =/* ap_info[i].rssi*/ -50 * 0.6;
                   // intensity = (coefficient * coefficient + 100) * 1000;
//                    triggerSignal();
//                    vTaskDelay(1000/portTICK_PERIOD_MS);
//                    ESP_ERROR_CHECK(esp_timer_stop(anomalySignalTimer));
//                    ESP_ERROR_CHECK(esp_timer_start_periodic(anomalySignalTimer, intensity));
               // }
            //}
        }
    }
}

static void wifiScan(void)
{
    uint16_t number = SCAN_LIST_SIZE;
    esp_wifi_scan_start(NULL, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&apCount));
}

static void triggerSignal()
{
    gpio_set_level(INDICATOR_GPIO, indicatorLevel);

    if (indicatorLevel > 0) {
        indicatorLevel = 0;
    } else {
        indicatorLevel = 1;
    }
}
