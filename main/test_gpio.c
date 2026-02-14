#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "GPIO_TEST";

#define PIN_MOSI  11
#define PIN_CLK   12
#define PIN_CS    10
#define PIN_DC    9
#define PIN_RST   14

void app_main(void)
{
    ESP_LOGI(TAG, "GPIO Test - Checking SPI pins");

    // Configure all pins as output
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_MOSI) | (1ULL << PIN_CLK) |
                        (1ULL << PIN_CS) | (1ULL << PIN_DC) | (1ULL << PIN_RST)
    };
    gpio_config(&io_conf);

    while(1) {
        ESP_LOGI(TAG, "Setting all pins HIGH");
        gpio_set_level(PIN_MOSI, 1);
        gpio_set_level(PIN_CLK, 1);
        gpio_set_level(PIN_CS, 1);
        gpio_set_level(PIN_DC, 1);
        gpio_set_level(PIN_RST, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Setting all pins LOW");
        gpio_set_level(PIN_MOSI, 0);
        gpio_set_level(PIN_CLK, 0);
        gpio_set_level(PIN_CS, 0);
        gpio_set_level(PIN_DC, 0);
        gpio_set_level(PIN_RST, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
