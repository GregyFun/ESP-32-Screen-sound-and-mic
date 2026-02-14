#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_heap_caps.h"
#include "lvgl.h"
#include "credentials.h"  // WiFi and API credentials (NOT versioned in Git)
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"

// Custom font with French accented characters (generated with lv_font_conv)
LV_FONT_DECLARE(lv_font_montserrat_32_fr);

static const char *TAG = "S3_DISPLAY";

// Google Cloud TTS Configuration
#define TTS_API_URL "https://texttospeech.googleapis.com/v1/text:synthesize"
#define TTS_BUFFER_SIZE (512 * 1024)  // 512KB buffer for TTS audio (PSRAM)

// WiFi event group
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

// Pin configuration
#define LCD_HOST       SPI2_HOST

// LCD SPI Pins
#define PIN_NUM_MOSI   11    // SDI
#define PIN_NUM_CLK    12    // SCK
#define PIN_NUM_CS     10    // LCD_CS
#define PIN_NUM_DC     9     // LCD_RS (Data/Command)
#define PIN_NUM_RST    14    // LCD_RST
// MISO not connected - not needed for display output
// Backlight not connected - display works without it

// Audio I2S Pins (MAX98357A)
// Configuration validée pour ESP32-S3 (sources forum ESP32)
#define I2S_BCLK       21    // Bit Clock (GPIO 21 testé sur ESP32-S3)
#define I2S_LRC        17    // Word Select (GPIO 17 testé sur ESP32-S3)
#define I2S_DIN        16    // Data (GPIO 16 testé sur ESP32-S3)

// LCD resolution
#define LCD_H_RES      480
#define LCD_V_RES      320

// LVGL draw buffer size (1/10 of screen)
#define LVGL_DRAW_BUF_SIZE  (LCD_H_RES * LCD_V_RES / 10)

// Audio configuration
#define SAMPLE_RATE    24000
#define I2S_NUM        I2S_NUM_0

static spi_device_handle_t spi;
static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t disp_buf;
static lv_color_t *buf1;
static lv_color_t *buf2;
static i2s_chan_handle_t tx_chan;
static lv_obj_t *ui_label = NULL;

// WiFi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP (%d/%d)", s_retry_num, WIFI_MAXIMUM_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "Connect to the AP failed");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Initialize WiFi in station mode
static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization finished.");

    // Wait until either connected or failed
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID: %s", WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID: %s", WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

// Send a command to the LCD
static void lcd_cmd(const uint8_t cmd)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    gpio_set_level(PIN_NUM_DC, 0);  // Command mode
    spi_device_polling_transmit(spi, &t);
}

// Send data to the LCD
static void lcd_data(const uint8_t *data, int len)
{
    spi_transaction_t t;
    if (len == 0) return;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;
    t.tx_buffer = data;
    gpio_set_level(PIN_NUM_DC, 1);  // Data mode
    spi_device_polling_transmit(spi, &t);
}

// Convert RGB565 to RGB666 (18-bit) for ILI9488 SPI
static void rgb565_to_rgb666(uint16_t color, uint8_t *rgb)
{
    // Extract RGB565 components
    uint8_t r = (color >> 11) & 0x1F;  // 5 bits
    uint8_t g = (color >> 5) & 0x3F;   // 6 bits
    uint8_t b = color & 0x1F;          // 5 bits

    // Expand to 6 bits each for RGB666
    rgb[0] = (r << 1) | (r >> 4);      // Red
    rgb[1] = g;                         // Green (already 6 bits)
    rgb[2] = (b << 1) | (b >> 4);      // Blue
}

// Initialize the ILI9488 IPS
static void ili9488_init(void)
{
    // Hardware reset
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(150));

    // Software reset (CRITICAL - must be first command)
    lcd_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Exit sleep mode
    lcd_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));

    // Positive Gamma Control
    lcd_cmd(0xE0);
    uint8_t gamma_p[] = {0x00, 0x07, 0x0F, 0x0D, 0x1B, 0x0A, 0x3C, 0x78, 0x4A, 0x07, 0x0E, 0x09, 0x1B, 0x1E, 0x0F};
    lcd_data(gamma_p, sizeof(gamma_p));

    // Negative Gamma Control
    lcd_cmd(0xE1);
    uint8_t gamma_n[] = {0x00, 0x22, 0x24, 0x06, 0x12, 0x07, 0x36, 0x47, 0x47, 0x06, 0x0A, 0x07, 0x30, 0x37, 0x0F};
    lcd_data(gamma_n, sizeof(gamma_n));

    // Power Control 1
    lcd_cmd(0xC0);
    uint8_t pwr1[] = {0x10, 0x10};
    lcd_data(pwr1, sizeof(pwr1));

    // Power Control 2
    lcd_cmd(0xC1);
    uint8_t pwr2[] = {0x41};
    lcd_data(pwr2, sizeof(pwr2));

    // VCOM Control (IPS specific: -0.79688V)
    lcd_cmd(0xC5);
    uint8_t vcom[] = {0x00, 0x4D, 0x80};
    lcd_data(vcom, sizeof(vcom));

    // Memory Access Control (IPS specific)
    lcd_cmd(0x36);
    uint8_t madctl[] = {0x28};  // MY=0, MX=1, MV=0, RGB (not BGR for IPS)
    lcd_data(madctl, sizeof(madctl));

    // Pixel Format Set - 18bit (CRITICAL for SPI interface!)
    lcd_cmd(0x3A);
    uint8_t pixfmt[] = {0x66};  // 18 bits/pixel (RGB666)
    lcd_data(pixfmt, sizeof(pixfmt));

    // Interface Mode Control (important for IPS)
    lcd_cmd(0xB0);
    uint8_t ifmode[] = {0x00};  // Use SDI
    lcd_data(ifmode, sizeof(ifmode));

    // Frame Rate Control (Normal mode)
    lcd_cmd(0xB1);
    uint8_t frmctr[] = {0xB0, 0x11};  // 70Hz
    lcd_data(frmctr, sizeof(frmctr));

    // Display Inversion Control (IPS needs inversion ON)
    lcd_cmd(0xB4);
    uint8_t invctr[] = {0x02};  // 2-dot inversion
    lcd_data(invctr, sizeof(invctr));

    // Display Function Control
    lcd_cmd(0xB6);
    uint8_t dfunctr[] = {0x02, 0x02, 0x3B};
    lcd_data(dfunctr, sizeof(dfunctr));

    // Entry Mode Set
    lcd_cmd(0xB7);
    uint8_t entry[] = {0xC6};
    lcd_data(entry, sizeof(entry));

    // Adjust Control 3
    lcd_cmd(0xF7);
    uint8_t adj3[] = {0xA9, 0x51, 0x2C, 0x82};
    lcd_data(adj3, sizeof(adj3));

    // Inversion ON (critical for IPS!)
    lcd_cmd(0x21);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Normal Display Mode ON
    lcd_cmd(0x13);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Display on
    lcd_cmd(0x29);
    vTaskDelay(pdMS_TO_TICKS(150));

    ESP_LOGI(TAG, "ILI9488 IPS initialized");
}

// Set drawing window
static void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    // Column Address Set
    lcd_cmd(0x2A);
    uint8_t col_data[] = {x1 >> 8, x1 & 0xFF, x2 >> 8, x2 & 0xFF};
    lcd_data(col_data, 4);

    // Page Address Set
    lcd_cmd(0x2B);
    uint8_t row_data[] = {y1 >> 8, y1 & 0xFF, y2 >> 8, y2 & 0xFF};
    lcd_data(row_data, 4);

    // Memory Write
    lcd_cmd(0x2C);
}

// LVGL flush callback - sends framebuffer to display
// Uses a small static DMA buffer to convert and send pixels in chunks,
// avoiding large DMA allocations that can fail when internal RAM is low.
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    // Set window to the area to update
    lcd_set_window(area->x1, area->y1, area->x2, area->y2);

    // Calculate the number of pixels
    uint32_t width = area->x2 - area->x1 + 1;
    uint32_t height = area->y2 - area->y1 + 1;
    uint32_t num_pixels = width * height;

    // Small DMA buffer - process pixels in chunks of ~1365 pixels (4095 bytes RGB666)
    // This avoids allocating a huge DMA buffer for the entire area
    #define FLUSH_CHUNK_PIXELS 1365
    static uint8_t *rgb_chunk = NULL;
    if (rgb_chunk == NULL) {
        rgb_chunk = heap_caps_malloc(FLUSH_CHUNK_PIXELS * 3, MALLOC_CAP_DMA);
        if (rgb_chunk == NULL) {
            ESP_LOGE(TAG, "Failed to allocate RGB chunk buffer");
            lv_disp_flush_ready(drv);
            return;
        }
    }

    gpio_set_level(PIN_NUM_DC, 1);  // Data mode

    uint32_t pixels_sent = 0;
    while (pixels_sent < num_pixels) {
        uint32_t chunk_pixels = num_pixels - pixels_sent;
        if (chunk_pixels > FLUSH_CHUNK_PIXELS) {
            chunk_pixels = FLUSH_CHUNK_PIXELS;
        }

        // Convert this chunk from RGB565 to RGB666
        for (uint32_t i = 0; i < chunk_pixels; i++) {
            uint16_t color565 = color_map[pixels_sent + i].full;
            rgb565_to_rgb666(color565, &rgb_chunk[i * 3]);
        }

        // Send this chunk via SPI
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = chunk_pixels * 3 * 8;
        t.tx_buffer = rgb_chunk;
        spi_device_polling_transmit(spi, &t);

        pixels_sent += chunk_pixels;
    }

    // Tell LVGL we're done
    lv_disp_flush_ready(drv);
}

// LVGL tick timer callback
static void lvgl_tick_timer_cb(void *arg)
{
    lv_tick_inc(10);  // 10ms tick
}

// Initialize LVGL
static void lvgl_init(void)
{
    ESP_LOGI(TAG, "Initializing LVGL...");

    // Initialize LVGL
    lv_init();

    // Allocate draw buffers in PSRAM (faster and more space)
    buf1 = heap_caps_malloc(LVGL_DRAW_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    buf2 = heap_caps_malloc(LVGL_DRAW_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

    if (buf1 == NULL || buf2 == NULL) {
        ESP_LOGE(TAG, "Failed to allocate LVGL buffers");
        return;
    }

    // Initialize display buffer
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LVGL_DRAW_BUF_SIZE);

    // Register display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    // Create periodic timer for LVGL tick (10ms)
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lvgl_tick_timer_cb,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 10 * 1000));  // 10ms

    ESP_LOGI(TAG, "LVGL initialized");
}

// Initialize I2S for MAX98357A
static void i2s_init(void)
{
    ESP_LOGI(TAG, "Initializing I2S for MAX98357A...");

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK,
            .ws = I2S_LRC,
            .dout = I2S_DIN,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));

    ESP_LOGI(TAG, "I2S initialized (16kHz, 16-bit, Mono)");
}

// Play a loud test tone (disabled - uncomment to test speaker)
#if 0
static void play_test_tone(void)
{
    ESP_LOGI(TAG, "Playing LOUD test tone - 5 beeps of 880Hz...");

    const int beep_duration_ms = 500;   // 500ms per beep
    const int pause_duration_ms = 200;  // 200ms pause
    const int frequency = 880;          // Note A5 (higher pitch, more audible)
    const int num_beeps = 5;

    // Beep buffer
    const int beep_samples = (SAMPLE_RATE * beep_duration_ms) / 1000;
    int16_t *beep_buf = malloc(beep_samples * sizeof(int16_t));

    // Silence buffer
    const int pause_samples = (SAMPLE_RATE * pause_duration_ms) / 1000;
    int16_t *silence_buf = calloc(pause_samples, sizeof(int16_t)); // All zeros

    if (beep_buf == NULL || silence_buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer");
        free(beep_buf);
        free(silence_buf);
        return;
    }

    // Generate LOUD sine wave (MAX volume!)
    const float amplitude = 32000.0f;  // Near maximum (32767)
    for (int i = 0; i < beep_samples; i++) {
        float t = (float)i / SAMPLE_RATE;
        beep_buf[i] = (int16_t)(amplitude * sinf(2.0f * M_PI * frequency * t));
    }

    // Play beeps
    for (int beep = 0; beep < num_beeps; beep++) {
        ESP_LOGI(TAG, "Beep %d/%d...", beep + 1, num_beeps);

        // Send beep
        size_t bytes_written = 0;
        i2s_channel_write(tx_chan, beep_buf, beep_samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);

        // Send silence (pause)
        i2s_channel_write(tx_chan, silence_buf, pause_samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    }

    free(beep_buf);
    free(silence_buf);
    ESP_LOGI(TAG, "Test tone complete!");
}
#endif

// Buffer for HTTP response
typedef struct {
    char *buffer;
    int buffer_len;
    int data_len;
} http_response_t;

// HTTP event handler for TTS
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_response_t *response = (http_response_t *)evt->user_data;

    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // Accumulate data in buffer
            if (response->buffer == NULL) {
                response->buffer = heap_caps_malloc(TTS_BUFFER_SIZE, MALLOC_CAP_SPIRAM);
                response->buffer_len = TTS_BUFFER_SIZE;
                response->data_len = 0;
            }
            if (response->data_len + evt->data_len < response->buffer_len) {
                memcpy(response->buffer + response->data_len, evt->data, evt->data_len);
                response->data_len += evt->data_len;
            } else {
                ESP_LOGE(TAG, "TTS buffer overflow! Need more than %d bytes", response->buffer_len);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            break;
    }
    return ESP_OK;
}

// Google Cloud TTS: Synthesize speech from text
static char* google_tts_synthesize(const char *text)
{
    if (strlen(GOOGLE_TTS_API_KEY) == 0) {
        ESP_LOGE(TAG, "Google TTS API Key is empty! Please add it to credentials.h");
        return NULL;
    }

    ESP_LOGI(TAG, "Calling Google Cloud TTS API for: \"%s\"", text);

    // Build JSON request
    cJSON *root = cJSON_CreateObject();
    cJSON *input = cJSON_CreateObject();
    cJSON_AddStringToObject(input, "text", text);
    cJSON_AddItemToObject(root, "input", input);

    cJSON *voice = cJSON_CreateObject();
    cJSON_AddStringToObject(voice, "languageCode", "fr-FR");  // French voice
    cJSON_AddStringToObject(voice, "name", "fr-FR-Standard-A");  // Female voice
    cJSON_AddItemToObject(root, "voice", voice);

    cJSON *audioConfig = cJSON_CreateObject();
    cJSON_AddStringToObject(audioConfig, "audioEncoding", "LINEAR16");  // PCM audio, no decoder needed!
    cJSON_AddNumberToObject(audioConfig, "sampleRateHertz", SAMPLE_RATE);
    cJSON_AddItemToObject(root, "audioConfig", audioConfig);

    char *json_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    ESP_LOGI(TAG, "Request JSON: %s", json_data);

    // Build URL with API key
    char url[256];
    snprintf(url, sizeof(url), "%s?key=%s", TTS_API_URL, GOOGLE_TTS_API_KEY);

    // Prepare response buffer
    http_response_t response = {
        .buffer = NULL,
        .buffer_len = 0,
        .data_len = 0
    };

    // Configure HTTP client
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .user_data = &response,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
        .buffer_size = 4096,  // Internal buffer for chunked reads
        .crt_bundle_attach = esp_crt_bundle_attach,  // Enable certificate verification
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Set headers
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    // Perform request
    esp_err_t err = esp_http_client_perform(client);

    char *audio_content = NULL;

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Status = %d, content_length = %lld",
                status_code,
                esp_http_client_get_content_length(client));

        if (status_code == 200 && response.buffer != NULL && response.data_len > 0) {
            // Null-terminate the response
            response.buffer[response.data_len] = '\0';

            ESP_LOGI(TAG, "Response received: %d bytes", response.data_len);

            // Parse JSON response
            cJSON *response_json = cJSON_Parse(response.buffer);
            if (response_json) {
                cJSON *audio_content_json = cJSON_GetObjectItem(response_json, "audioContent");
                if (audio_content_json && cJSON_IsString(audio_content_json)) {
                    size_t b64_len = strlen(audio_content_json->valuestring);
                    audio_content = heap_caps_malloc(b64_len + 1, MALLOC_CAP_SPIRAM);
                    if (audio_content) {
                        memcpy(audio_content, audio_content_json->valuestring, b64_len + 1);
                        ESP_LOGI(TAG, "Got audio content (base64 length: %d)", b64_len);
                    } else {
                        ESP_LOGE(TAG, "Failed to allocate PSRAM for audio content (%d bytes)", b64_len);
                    }
                } else {
                    ESP_LOGE(TAG, "audioContent not found in response");
                }
                cJSON_Delete(response_json);
            } else {
                ESP_LOGE(TAG, "Failed to parse JSON response");
                ESP_LOGE(TAG, "Raw response: %s", response.buffer);
            }
        } else {
            ESP_LOGE(TAG, "HTTP request failed with status: %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }

    // Free response buffer
    if (response.buffer) {
        free(response.buffer);
    }

    esp_http_client_cleanup(client);
    free(json_data);

    return audio_content;  // Caller must free
}

// Simple base64 decode (for MP3 audio from Google TTS)
static size_t base64_decode(const char *input, uint8_t **output)
{
    static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    size_t input_len = strlen(input);
    size_t output_len = (input_len / 4) * 3;

    // Remove padding
    if (input[input_len - 1] == '=') output_len--;
    if (input[input_len - 2] == '=') output_len--;

    *output = heap_caps_malloc(output_len, MALLOC_CAP_SPIRAM);
    if (*output == NULL) {
        ESP_LOGE(TAG, "Failed to allocate PSRAM for base64 decode (%d bytes)", output_len);
        return 0;
    }

    size_t j = 0;
    uint32_t buffer = 0;
    int bits = 0;

    for (size_t i = 0; i < input_len; i++) {
        if (input[i] == '=') break;

        const char *p = strchr(base64_table, input[i]);
        if (p == NULL) continue;  // Skip invalid characters

        buffer = (buffer << 6) | (p - base64_table);
        bits += 6;

        if (bits >= 8) {
            bits -= 8;
            (*output)[j++] = (buffer >> bits) & 0xFF;
        }
    }

    return j;
}

// Play LINEAR16 (PCM) audio via I2S
static void play_audio_pcm(const uint8_t *audio_data, size_t audio_len)
{
    if (audio_len == 0 || audio_data == NULL) {
        ESP_LOGE(TAG, "Invalid audio data");
        return;
    }

    // Google TTS LINEAR16 returns a WAV file - skip the 44-byte header
    size_t wav_header_size = 0;
    if (audio_len > 44 && memcmp(audio_data, "RIFF", 4) == 0) {
        wav_header_size = 44;
        ESP_LOGI(TAG, "WAV header detected, skipping %d bytes", wav_header_size);
    }
    const uint8_t *pcm_start = audio_data + wav_header_size;
    size_t pcm_len = audio_len - wav_header_size;

    ESP_LOGI(TAG, "Playing LINEAR16 audio (%d bytes = %d samples)...",
             pcm_len, pcm_len / 2);

    // Make a copy of the audio data to apply fade-in/fade-out
    int16_t *audio_copy = heap_caps_malloc(pcm_len, MALLOC_CAP_SPIRAM);
    if (audio_copy == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for audio copy");
        return;
    }
    memcpy(audio_copy, pcm_start, pcm_len);

    // Apply fade-in to avoid click/pop at start (first 50ms)
    const int fade_samples = (SAMPLE_RATE * 50) / 1000;  // 50ms fade-in
    int num_samples = pcm_len / 2;
    for (int i = 0; i < fade_samples && i < num_samples; i++) {
        float factor = (float)i / fade_samples;  // 0.0 to 1.0
        audio_copy[i] = (int16_t)(audio_copy[i] * factor);
    }

    // Apply fade-out to avoid click/pop at end (last 50ms)
    for (int i = 0; i < fade_samples && i < num_samples; i++) {
        int idx = num_samples - 1 - i;
        float factor = (float)i / fade_samples;  // 0.0 to 1.0
        audio_copy[idx] = (int16_t)(audio_copy[idx] * factor);
    }

    // Play the audio via I2S
    size_t bytes_written = 0;
    esp_err_t err = i2s_channel_write(tx_chan, audio_copy, pcm_len, &bytes_written, portMAX_DELAY);

    free(audio_copy);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Audio playback complete! (%d bytes written)", bytes_written);
    } else {
        ESP_LOGE(TAG, "I2S write failed: %s", esp_err_to_name(err));
    }
}

// Text-to-Speech
static void tts_say(const char *text)
{
    ESP_LOGI(TAG, "=== Starting TTS: \"%s\" ===", text);

    // Call Google Cloud TTS API
    char *audio_base64 = google_tts_synthesize(text);

    if (audio_base64) {
        ESP_LOGI(TAG, "TTS API call successful!");

        // Decode base64 to LINEAR16 (PCM) binary
        uint8_t *pcm_data = NULL;
        size_t pcm_len = base64_decode(audio_base64, &pcm_data);

        if (pcm_data && pcm_len > 0) {
            ESP_LOGI(TAG, "Base64 decoded: %d bytes of LINEAR16 PCM audio", pcm_len);

            // Play the audio via I2S
            play_audio_pcm(pcm_data, pcm_len);

            free(pcm_data);
        } else {
            ESP_LOGE(TAG, "Failed to decode base64 audio");
        }

        free(audio_base64);
    } else {
        ESP_LOGE(TAG, "TTS API call failed - Check API key in credentials.h");
    }

    ESP_LOGI(TAG, "=== TTS Complete ===");
}

// Display text on screen and speak it
static void display_and_say(const char *text)
{
    if (ui_label) {
        lv_label_set_text(ui_label, text);
        lv_obj_align(ui_label, LV_ALIGN_CENTER, 0, 0);
    }
    tts_say(text);
}

// Proverbes francais (UTF-8 pour le TTS)
static const char *proverbes[] = {
    "Qui s\xc3\xa8me le vent, r\xc3\xa9" "colte la temp\xc3\xaate.",
    "Pierre qui roule n'amasse pas mousse.",
    "L'habit ne fait pas le moine.",
    "Petit \xc3\xa0 petit, l'oiseau fait son nid.",
    "Qui vivra verra.",
    "Mieux vaut tard que jamais.",
    "Apr\xc3\xa8s la pluie, le beau temps.",
    "Chat \xc3\xa9" "chaud\xc3\xa9 craint l'eau froide.",
    "Tout ce qui brille n'est pas or.",
    "La nuit porte conseil.",
    "Il ne faut pas vendre la peau de l'ours avant de l'avoir tu\xc3\xa9.",
    "Qui ne tente rien n'a rien.",
    "Les murs ont des oreilles.",
    "L'app\xc3\xa9tit vient en mangeant.",
    "Chien qui aboie ne mord pas.",
    "\xc3\x80 bon chat, bon rat.",
    "Les bons comptes font les bons amis.",
    "Un tiens vaut mieux que deux tu l'auras.",
    "Rira bien qui rira le dernier.",
    "Rien ne sert de courir, il faut partir \xc3\xa0 point.",
};
static const int num_proverbes = sizeof(proverbes) / sizeof(proverbes[0]);

// FreeRTOS task for TTS (runs on separate core)
static void tts_task(void *pvParameters)
{
    // Wait for WiFi to connect before calling TTS
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(TAG, "TTS task started");

    int last_index = -1;
    while (1) {
        // Pick a random proverb (avoid repeating the same one)
        int index;
        do {
            index = esp_random() % num_proverbes;
        } while (index == last_index);
        last_index = index;

        display_and_say(proverbes[index]);

        // Wait 30 seconds before next proverb
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

// Create UI
static void create_ui(void)
{
    ui_label = lv_label_create(lv_scr_act());

    lv_label_set_text(ui_label, "Chargement...");
    lv_label_set_long_mode(ui_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ui_label, LCD_H_RES - 40);

    lv_obj_set_style_text_font(ui_label, &lv_font_montserrat_32_fr, 0);
    lv_obj_align(ui_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(ui_label, lv_color_hex(0x0000FF), 0);

    ESP_LOGI(TAG, "Hello World UI created");
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-S3 + ILI9488 IPS + LVGL + Audio + WiFi Test");

    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize WiFi
    ESP_LOGI(TAG, "Initializing WiFi...");
    wifi_init_sta();

    // Configure GPIO for reset and DC pins
    gpio_config_t gpio_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_NUM_RST) | (1ULL << PIN_NUM_DC)
    };
    gpio_config(&gpio_conf);
    gpio_set_level(PIN_NUM_RST, 1);

    // Initialize SPI bus (MISO not needed for display output)
    ESP_LOGI(TAG, "Initializing SPI bus...");
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,  // Not connected
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * 3  // Enough for multiple lines in RGB666
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // Attach the LCD to the SPI bus
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000,  // 10 MHz
        .mode = 0,  // SPI mode 0
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
        .flags = SPI_DEVICE_HALFDUPLEX,  // Half-duplex mode
        .input_delay_ns = 0,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_HOST, &devcfg, &spi));

    // Initialize ILI9488 IPS
    ESP_LOGI(TAG, "Initializing ILI9488 IPS...");
    ili9488_init();

    // Initialize LVGL
    lvgl_init();

    // Create Hello World UI
    create_ui();

    // Initialize I2S audio
    i2s_init();

    ESP_LOGI(TAG, "Setup complete!");

    // Launch TTS in a separate task (32KB stack)
    xTaskCreate(tts_task, "tts_task", 32768, NULL, 5, NULL);

    // LVGL task handler loop (runs immediately on core 0)
    while (1) {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
