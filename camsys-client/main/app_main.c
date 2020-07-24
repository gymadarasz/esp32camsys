#include <stdio.h>
#include "sdkconfig.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_vfs_common.h"
#include "esp_vfs_dev.h"
#include "esp_websocket_client.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "hal/uart_types.h"
#include "driver/uart.h"
#include "driver/sdmmc_types.h"
#include "driver/sdspi_host.h"


#define CAMSYS_NAMESPACE "camsys"

static const char *TAG = CAMSYS_NAMESPACE;

// ---------------------------------------------------------------
// DELAY
// ---------------------------------------------------------------

void delay(long ms) {
    vTaskDelay(ms / portTICK_RATE_MS);
}

// ---------------------------------------------------------------
// ERROR
// ---------------------------------------------------------------

#define ERROR_BUFF_SIZE 100

#define ERROR(msg) errorlnf(__LINE__, msg)
#define ERRORF(fmt, ...) errorlnf(__LINE__, fmt, __VA_ARGS__)

void errorlnf(int line, const char* fmt, ...) {
    char buffer[ERROR_BUFF_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    printf("ERROR at line %d: %s\n", line, buffer);

    fflush(stdout);
    delay(1000);
    esp_restart();
}

// ---------------------------------------------------------------
// SERIAL READ
// ---------------------------------------------------------------

#define SERIAL_READLN_BUFF_SIZE 255

esp_err_t serial_install(uart_port_t port, esp_line_endings_t in_line_ending, esp_line_endings_t out_line_ending) {
    // Initialize VFS & UART so we can use std::cout/cin
    if (setvbuf(stdin, NULL, _IONBF, 0)) return ESP_FAIL;
    /* Install UART driver for interrupt-driven reads and writes */
    esp_err_t err = uart_driver_install(port, 256, 0, 0, NULL, 0);
    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(port);
    esp_vfs_dev_uart_set_rx_line_endings(in_line_ending);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(out_line_ending);
    return err;
}

esp_err_t serial_uninstall(uart_port_t port) {
    return uart_driver_delete(port);
}

char* serial_readln() {
    char* str = calloc(sizeof(char), SERIAL_READLN_BUFF_SIZE + 1);
    if (!fgets(str, SERIAL_READLN_BUFF_SIZE, stdin)) {
        free(str);
        return NULL;
    }
    str[strlen(str)-1] = '\0';
    return str;
}

// ---------------------------------------------------------------
// SDCARD READ
// ---------------------------------------------------------------

#define SDCARD_TARGET_ESP32S2
//#define SDCARD_TARGET_ESP32

#ifdef SDCARD_TARGET_ESP32
#include "driver/sdmmc_host.h"
#endif

#define SDCARD_MOUNT_POINT "/sdcard"

// This example can use SDMMC and SPI peripherals to communicate with SD card.
// By default, SDMMC peripheral is used.
// To enable SPI mode, uncomment the following line:

// #define SDCARD_USE_SPI_MODE

// ESP32-S2 doesn't have an SD Host peripheral, always use SPI:
#ifdef SDCARD_TARGET_ESP32S2
#ifndef SDCARD_USE_SPI_MODE
#define SDCARD_USE_SPI_MODE
#endif // SDCARD_USE_SPI_MODE
// on ESP32-S2, DMA channel must be the same as host id
#define SDCARD_SPI_DMA_CHAN    host.slot
#endif //CONFIG_IDF_TARGET_ESP32S2

// DMA channel to be used by the SPI peripheral
#ifndef SDCARD_SPI_DMA_CHAN
#define SDCARD_SPI_DMA_CHAN    1
#endif //SDCARD_SPI_DMA_CHAN

// When testing SD and SPI modes, keep in mind that once the card has been
// initialized in SPI mode, it can not be reinitialized in SD mode without
// toggling power to the card.

#ifdef SDCARD_USE_SPI_MODE
// Pin mapping when using SPI mode.
// With this mapping, SD card can be used both in SPI and 1-line SD mode.
// Note that a pull-up on CS line is required in SD mode.
#define SDCARD_PIN_NUM_MISO 2
#define SDCARD_PIN_NUM_MOSI 15
#define SDCARD_PIN_NUM_CLK  14
#define SDCARD_PIN_NUM_CS   13
#endif //SDCARD_USE_SPI_MODE

struct sdcard_s {
    sdmmc_card_t* card;
    int slot;
};

typedef struct sdcard_s sdcard_t;

/**
 * @brief Configuration arguments for esp_vfs_fat_sdmmc_mount and esp_vfs_fat_spiflash_mount functions
 */
typedef struct {
    bool format_if_mount_failed;
    int max_files;
    size_t allocation_unit_size;
} sdcard_mount_config_t;

typedef esp_err_t (*sdcard_mount_func_t)(
    const char* base_path,
    const sdmmc_host_t* host_config_input,
    const sdspi_device_config_t* slot_config,
    const sdcard_mount_config_t* mount_config,
    sdmmc_card_t** out_card
);

typedef esp_err_t (*sdcard_unmount_func_t)(const char *base_path, sdmmc_card_t *card);

sdcard_t sdcard = {NULL, 0};

esp_err_t sdcard_init(sdcard_mount_func_t mounter, bool auto_format, int max_files, size_t alloc_unit_size) {
    esp_err_t ret = ESP_OK;
    sdcard_mount_config_t mount_config;
    mount_config.format_if_mount_failed = auto_format;
    mount_config.max_files = max_files;
    mount_config.allocation_unit_size = alloc_unit_size;
    
    const char mount_point[] = SDCARD_MOUNT_POINT;

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
#ifndef SDCARD_USE_SPI_MODE
    // ESP_LOGI("sdcard", "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // To use 1-line SD mode, uncomment the following line:
    // slot_config.width = 1;

    // GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
    // Internal pull-ups are not sufficient. However, enabling internal pull-ups
    // does make a difference some boards, so we do that here.
    gpio_set_pull_mode(15, GPIO_PULLUP_ONLY);   // CMD, needed in 4- and 1- line modes
    gpio_set_pull_mode(2, GPIO_PULLUP_ONLY);    // D0, needed in 4- and 1-line modes
    gpio_set_pull_mode(4, GPIO_PULLUP_ONLY);    // D1, needed in 4-line mode only
    gpio_set_pull_mode(12, GPIO_PULLUP_ONLY);   // D2, needed in 4-line mode only
    gpio_set_pull_mode(13, GPIO_PULLUP_ONLY);   // D3, needed in 4- and 1-line modes

    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &sdcard.card);
#else
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SDCARD_PIN_NUM_MOSI,
        .miso_io_num = SDCARD_PIN_NUM_MISO,
        .sclk_io_num = SDCARD_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SDCARD_SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize sdcard bus.");
        sdcard.slot = host.slot;
        return ret;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SDCARD_PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ret = mounter(mount_point, &host, &slot_config, &mount_config, &sdcard.card);
#endif //USE_SPI_MODE

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE("sdcard", "Failed to mount filesystem. "
                /*"If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option."*/);
        } else {
            ESP_LOGE("sdcard", "Failed to initialize the card (%s). "
                /*"Make sure SD card lines have pull-up resistors in place."*/, esp_err_to_name(ret));
        }
        sdcard.slot = host.slot;
        return ret;
    }

    // Card has been initialized, print its properties
    // sdmmc_card_print_info(stdout, card);

    sdcard.slot = host.slot;
    return ret;
}

esp_err_t sdcard_close(sdcard_unmount_func_t unmounter) {
    // All done, unmount partition and disable SDMMC or SPI peripheral
    esp_err_t err_vfs = unmounter(SDCARD_MOUNT_POINT, sdcard.card);
    esp_err_t err_spi = ESP_OK;
#ifdef SDCARD_USE_SPI_MODE
    //deinitialize the bus after all devices are removed
    err_spi = spi_bus_free(sdcard.slot);
#endif
    return err_vfs ? err_vfs : err_spi;
}

// ---------------------------------------------------------------
// STRING EXTRAS
// ---------------------------------------------------------------

char* strncpy_offs(char* dest, size_t offs, const char* src, size_t max) {
    int i=0;
    for (; i+offs<max; i++) {
        if (!src[i]) break;
        dest[i+offs] = src[i];
    }
    dest[i+offs] = '\0';
    return dest;
}

// ---------------------------------------------------------------
// WIFI CREDENTIALS
// ---------------------------------------------------------------


#define WIFI_CREDENTIALS 10
#define WIFI_SSID_SIZE 32
#define WIFI_PSWD_SIZE 64
#define WIFI_CRED_SIZE ((WIFI_SSID_SIZE) + (WIFI_PSWD_SIZE))
#define WIFI_CREDS_SIZE ((WIFI_CREDENTIALS) * (WIFI_CRED_SIZE))


#define WIFI_CREDS_SET(creds, i, ssid, pswd) { \
    strncpy_offs(creds, i*WIFI_CRED_SIZE, ssid, WIFI_CREDS_SIZE); \
    strncpy_offs(creds, i*WIFI_CRED_SIZE+WIFI_SSID_SIZE, pswd, WIFI_CREDS_SIZE); \
}

#define WIFI_CREDS_GET_SSID(creds, i) (&creds[i * (WIFI_CRED_SIZE)])
#define WIFI_CREDS_GET_PSWD(creds, i) (&creds[i * (WIFI_CRED_SIZE) + (WIFI_SSID_SIZE)])


typedef char wifi_creds_t[WIFI_CREDS_SIZE];


typedef void (*app_cb_func_t)(void* app);
typedef void (*websock_app_cb_func_t)(void* app, esp_websocket_event_data_t* data);


struct wscli_app_s {
    app_cb_func_t settings_func;
    app_cb_func_t loop_func;
    app_cb_func_t on_wifi_connected;
    app_cb_func_t on_wifi_disconnected;
    app_cb_func_t on_websock_connected;
    app_cb_func_t on_websock_disconnected;
    websock_app_cb_func_t on_websock_data;
    app_cb_func_t on_websock_error;
    app_cb_func_t on_websock_loop;
};

typedef struct wscli_app_s wscli_app_t;

struct wifi_app_s {
    const char* namespace;
    nvs_handle_t nvs_handle;
    wifi_creds_t wifi_creds;
    int mode;
    bool exited;
    wscli_app_t* ext;
};

typedef struct wifi_app_s wifi_app_t;


void wifi_creds_clear(wifi_creds_t creds) {
    for (int i=0; i<WIFI_CREDENTIALS; i++) WIFI_CREDS_SET(creds, i, "", "");
}

char* wifi_creds_get_pswd(wifi_creds_t creds, const char* ssid) {
    for (int i=0; i<WIFI_CREDENTIALS; i++) {
        if (!strcmp(WIFI_CREDS_GET_SSID(creds, i), ssid)) {
            return WIFI_CREDS_GET_PSWD(creds, i);
        }
    }
    return NULL;
}

esp_err_t wifi_creds_save(wifi_app_t* app) {
    // Write
    esp_err_t err = nvs_set_blob(app->nvs_handle, "wifi_creds", app->wifi_creds, WIFI_CREDS_SIZE);
    if (err == ESP_OK) err = nvs_commit(app->nvs_handle);
    
    return err;
}

esp_err_t wifi_creds_load(wifi_app_t* app) {
    // Read
    size_t length = WIFI_CREDS_SIZE;
    esp_err_t err = nvs_get_blob(app->nvs_handle, "wifi_creds", app->wifi_creds, &length);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        wifi_creds_clear(app->wifi_creds);
        length = WIFI_CREDS_SIZE;
        err = ESP_OK;
    }
    if (err == ESP_OK) if (length != WIFI_CREDS_SIZE) err = ESP_FAIL;
    
    return err;
}

// ---------------------------------------------------------------
// FLASH
// ---------------------------------------------------------------

void flash_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

// ---------------------------------------------------------------
// GPIO
// ---------------------------------------------------------------

void gpio_pins_init(uint64_t _pin_bit_mask) {
    gpio_config_t io_conf = {
        .pin_bit_mask = _pin_bit_mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK( gpio_config(&io_conf) );
}

// ---------------------------------------------------------------
// WIFI APP
// ---------------------------------------------------------------

#define WIFI_SCAN_LIST_SIZE 10

typedef wifi_ap_record_t wifi_scan_list_t[WIFI_SCAN_LIST_SIZE];

void wifi_init() {
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
}


uint16_t wifi_scan(wifi_scan_list_t ap_info) {
    ESP_ERROR_CHECK(esp_wifi_start());

    uint16_t number = WIFI_SCAN_LIST_SIZE;
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(&ap_info));

    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);

    ESP_ERROR_CHECK( esp_wifi_stop() );
    return ap_count;
}

#define WIFI_CONNECT_MAXIMUM_RETRY  5

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

static int wifi_retry_num = 0;
bool wifi_disconnected = true;
bool wifi_disconnected_prev = true;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (wifi_retry_num < WIFI_CONNECT_MAXIMUM_RETRY) {
            esp_wifi_connect();
            wifi_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP (%d)", wifi_retry_num);
        } else {
            wifi_retry_num = 0;
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGE(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        // ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_retry_num = 0;
        wifi_disconnected = false;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_event_handler_instance_t wifi_handle_instance_disconnect;


static void wifi_disconnect_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) 
{
    ESP_ERROR_CHECK(
        esp_event_handler_instance_unregister(
            WIFI_EVENT, 
            WIFI_EVENT_STA_DISCONNECTED, 
            wifi_handle_instance_disconnect
        )
    );

    wifi_disconnected = true;
}

esp_err_t wifi_connect(const char* ssid, const char* pswd) {
    esp_err_t ret = ESP_OK;

    wifi_event_group = xEventGroupCreate();

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

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_STA_DISCONNECTED,
                                                        &wifi_disconnect_event_handler,
                                                        NULL,
                                                        &wifi_handle_instance_disconnect));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    strncpy((char*)wifi_config.sta.ssid, ssid, WIFI_SSID_SIZE);
    strncpy((char*)wifi_config.sta.password, pswd, WIFI_PSWD_SIZE);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        // ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
        //          ssid, pswd);
    } else if (bits & WIFI_FAIL_BIT) {
        // ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
        //          ssid, pswd);
        ret = ESP_ERR_WIFI_NOT_CONNECT;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        ret = ESP_FAIL;
        esp_restart();
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(wifi_event_group);

    if (ret != ESP_OK) ESP_ERROR_CHECK( esp_wifi_stop() );
    return ret;
}

esp_err_t wifi_scan_connect(wifi_creds_t creds) {
    wifi_retry_num = 0;

    wifi_scan_list_t ap_info;
    uint16_t ap_count = wifi_scan(ap_info);

    for (int i = 0; (i < WIFI_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        for (int j=0; j<WIFI_CREDENTIALS; j++) {
            // is ssid match?
            if (WIFI_CREDS_GET_SSID(creds, j)[0] && !strcmp(WIFI_CREDS_GET_SSID(creds, j), (char*)ap_info[i].ssid)) {
                // wifi connected?
                if (ESP_OK == wifi_connect(WIFI_CREDS_GET_SSID(creds, j), WIFI_CREDS_GET_PSWD(creds, j))) return ESP_OK;
            }
        }
    }
    return ESP_ERR_WIFI_NOT_CONNECT;
}

// ---------------------------------------------------------------
// WIFI CLIENT APP (loops)
// ---------------------------------------------------------------

void wifi_connection_establish(wifi_app_t* app) {
    while(ESP_OK != wifi_scan_connect(app->wifi_creds)) app->ext->loop_func(app);
}

void wifi_connection_keep_alive(wifi_app_t* app) {
    if (wifi_disconnected) wifi_connection_establish(app);
}

void wifi_app_run(wifi_app_t* app) {
    wifi_init();

    wifi_connection_establish(app);

    while(!app->exited) {
        wifi_connection_keep_alive(app);
        app->ext->loop_func(app);
    }
}

#define WIFI_APP_MODE_SETTING 0
#define WIFI_APP_MODE_RUN 1

int wifi_app_get_mode() {
    gpio_pins_init(1ULL<<GPIO_NUM_13);
    return gpio_get_level(GPIO_NUM_13) ? WIFI_APP_MODE_RUN : WIFI_APP_MODE_SETTING;
}

// --------------

/*
An example setup:

100 KEY
KF4GTX9
200 OK
100 SSID 10/1:
testwifi
200 OK
100 PSWD 10/1:
password1
200 OK
100 SSID 10/2:
secondwifi
200 OK
100 PSWD 10/2:
password2
200 OK
100 SSID 10/3:

200 OK
100 PSWD 10/3:

....

200 OK Credentials are saved.
100 HOST:
192.168.0.200
200 OK
*/

// TODO: Unique code for each device. Should be changed before flash!
#define WIFI_APP_SETTINGS_KEY "KF4GTX9"


void wifi_app_settings(wifi_app_t* app) {
    ESP_ERROR_CHECK( serial_install(UART_NUM_0, ESP_LINE_ENDINGS_CR, ESP_LINE_ENDINGS_CRLF) );

    printf("100 KEY\n");
    char* key = serial_readln();
    if (!strcmp(key, WIFI_APP_SETTINGS_KEY)) {
        free(key);

        wifi_creds_clear(app->wifi_creds);

        for (int i=1; i<=WIFI_CREDENTIALS; i++) {
            printf("200 OK\n");

            printf("100 SSID %d/%d:\n", WIFI_CREDENTIALS, i);
            char* ssid = serial_readln();
            printf("200 OK\n");

            printf("100 PSWD %d/%d:\n", WIFI_CREDENTIALS, i);
            char* pswd = serial_readln();
            WIFI_CREDS_SET(app->wifi_creds, i, ssid, pswd);
            free(ssid);
            free(pswd);            
        }

        esp_err_t err = wifi_creds_save(app);
        ESP_OK == err ?             
            printf("200 OK Credentials are saved.\n") :
            printf("300 ERROR Saving credential error: %s\n", esp_err_to_name(err));

        if (app->ext->settings_func) app->ext->settings_func(app);
    } else {
        free(key);
        printf("300 ERROR\n");
    }

    ESP_ERROR_CHECK( serial_uninstall(UART_NUM_0) );
}

// --------------



void wifi_app_main(wifi_app_t* app) {
    flash_init();

    // NVS Open
    ESP_ERROR_CHECK( nvs_open(app->namespace, NVS_READWRITE, &app->nvs_handle) );
    
    app->mode = wifi_app_get_mode();
    switch (app->mode) {
        case WIFI_APP_MODE_SETTING:
            wifi_app_settings(app);
            break;
        case WIFI_APP_MODE_RUN:
            ESP_ERROR_CHECK( wifi_creds_load(app) );
            wifi_app_run(app);
            break;
        default:
            ESP_LOGE(TAG, "Incorrect Wifi App Mode: %d", app->mode);
            esp_restart();
            break;
    }

    // NVS Close
    nvs_close(app->nvs_handle);
}

// ---------------------------------------------------------------
// WEBSOCKET CLIENT APP (settings)
// ---------------------------------------------------------------

esp_err_t wscli_app_host_save(nvs_handle_t handle, const char* host) {
    // Write
    esp_err_t err = nvs_set_str(handle, "host", host);
    if (err == ESP_OK) err = nvs_commit(handle);
    
    return err;
}

esp_err_t wscli_app_host_load(nvs_handle_t handle, char* host, size_t length) {
    size_t required_size;
    esp_err_t err = nvs_get_str(handle, "host", NULL, &required_size);
    if (err == ESP_OK) {
        if (required_size >= length) return ESP_ERR_NVS_INVALID_LENGTH;
        err = nvs_get_str(handle, "host", host, &required_size);
    }
    return err;
}

void wscli_app_settings(void* arg) {
    wifi_app_t* app = arg;
    printf("100 HOST:\n");
    char* host = serial_readln();
    esp_err_t err = wscli_app_host_save(app->nvs_handle, host);
    ESP_OK == err ?             
        printf("200 OK\n") :
        printf("300 ERROR - %s\n", esp_err_to_name(err));
    free(host);
}

// ---------------------------------------------------------------
// WEBSOCKET CLIENT APP (loop)
// ---------------------------------------------------------------

#define WEBSOCK_STATUS_CONNECTED 1
#define WEBSOCK_STATUS_DISCONNECTED 0

#define NO_DATA_TIMEOUT_SEC 10
esp_websocket_client_config_t websocket_cfg = {};
static TimerHandle_t shutdown_signal_timer;
static SemaphoreHandle_t shutdown_sema;
esp_websocket_client_handle_t client;
const char* wsuri = "ws://192.168.0.200";

static void wscli_app_shutdown_signaler(TimerHandle_t xTimer)
{
    xSemaphoreGive(shutdown_sema);
}

static void wscli_app_websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    wifi_app_t* app = handler_args;
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        if (app->ext->on_websock_connected) (app->ext->on_websock_connected)(app);
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        if (app->ext->on_websock_disconnected) (app->ext->on_websock_disconnected)(app);
        break;
    case WEBSOCKET_EVENT_DATA:
        xTimerReset(shutdown_signal_timer, portMAX_DELAY);
        if (app->ext->on_websock_data) (app->ext->on_websock_data)(app, data);
        break;
    case WEBSOCKET_EVENT_ERROR:
        if (app->ext->on_websock_error) (app->ext->on_websock_error)(app);
        break;
    default:
        ESP_LOGE(TAG, "Websocket event unhandled: %d", event_id);
        esp_restart();
        break;
    }
}

void wscli_app_websocket_start(wifi_app_t* app) {
    
    memset(&websocket_cfg, 0, sizeof(websocket_cfg));

    shutdown_signal_timer = xTimerCreate("Websocket shutdown timer", NO_DATA_TIMEOUT_SEC * 1000 / portTICK_PERIOD_MS,
                                         pdFALSE, NULL, wscli_app_shutdown_signaler);
    shutdown_sema = xSemaphoreCreateBinary();

    //ESP_ERROR_CHECK_WITHOUT_ABORT( wscli_app_host_load(app->nvs_handle, wsuri, WSURI_LENGTH_MAX) );
    websocket_cfg.uri = wsuri;
    
    ESP_LOGI(TAG, "Connecting to %s...", websocket_cfg.uri);

    client = esp_websocket_client_init(&websocket_cfg);
    ESP_ERROR_CHECK_WITHOUT_ABORT( esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, wscli_app_websocket_event_handler, (void *)app) );

    ESP_ERROR_CHECK_WITHOUT_ABORT( esp_websocket_client_start(client) );
    xTimerStart(shutdown_signal_timer, portMAX_DELAY);
}

void wscli_app_websocket_stop(wifi_app_t* app) {
    xSemaphoreTake(shutdown_sema, portMAX_DELAY);
    ESP_ERROR_CHECK_WITHOUT_ABORT( esp_websocket_client_stop(client) );
    ESP_LOGI(TAG, "Websocket Stopped");
    ESP_ERROR_CHECK_WITHOUT_ABORT( esp_websocket_client_destroy(client) );
}



void wscli_app_loop(void* arg) {
    wifi_app_t* app = arg;
    if (wifi_disconnected_prev != wifi_disconnected) {
        ESP_LOGI(TAG, "WIFI STATUS CHANGED TO %s\n", wifi_disconnected ? "DESCONNECTED" : "CONNECTED");
        if (wifi_disconnected) wscli_app_websocket_stop(app); 
        else wscli_app_websocket_start(app);
        wifi_disconnected_prev = wifi_disconnected;
        if (wifi_disconnected && app->ext->on_wifi_disconnected) (app->ext->on_wifi_disconnected)(app);
        else if (!wifi_disconnected && app->ext->on_wifi_connected) (app->ext->on_wifi_connected)(app);
    } else {
        ESP_LOGI(TAG, "WIFI STATUS IS %s\n", wifi_disconnected ? "DESCONNECTED" : "CONNECTED");
        wifi_disconnected_prev = wifi_disconnected;
        if(app->ext->on_websock_loop) (app->ext->on_websock_loop)(app);
    }
}

// ------------------------------------------------------

void wscli_app_on_wifi_connected(void* arg) {
    wifi_app_t* app = arg;
    ESP_LOGI(TAG, "----------- [WIFI CONNECTED] -------------");
}

void wscli_app_on_wifi_disconnected(void* arg) {
    wifi_app_t* app = arg;
    ESP_LOGI(TAG, "----------- [WIFI DISCONNECTED] -------------");
}

void wscli_app_on_websock_connected(void* arg) {
    wifi_app_t* app = arg;
    ESP_LOGI(TAG, "----------- [WEBSOCKET CONNECTED] -------------");
}

void wscli_app_on_websock_disconnected(void* arg) {
    wifi_app_t* app = arg;
    ESP_LOGI(TAG, "----------- [WEBSOCKET DISCONNECTED] -------------");
}

void wscli_app_on_websock_data(void* arg, esp_websocket_event_data_t* data) {
    wifi_app_t* app = arg;
    ESP_LOGI(TAG, "----------- [WEBSOCKET DATA] -------------");

    ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
    ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
    ESP_LOGW(TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
    ESP_LOGW(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);

}

void wscli_app_on_websock_error(void* arg) {
    wifi_app_t* app = arg;
    ESP_LOGI(TAG, "----------- [WEBSOCKET ERROR] -------------");
}

void wscli_app_on_websock_loop(void* arg) {
    wifi_app_t* app = arg;
    ESP_LOGI(TAG, "----------- [WEBSOCKET LOOP] -------------");
    delay(1000);
}


// ------------------------------------------------------

void wscli_app_main(wscli_app_t* ext) {
    wifi_app_t app;
    app.namespace = CAMSYS_NAMESPACE;
    app.exited = false; 
    app.ext = ext;

    wifi_app_main(&app);
}

// -

void app_main(void)
{
    //esp_log_level_set("*", ESP_LOG_NONE);

    wscli_app_t ext;
    ext.settings_func = wscli_app_settings;
    ext.loop_func = wscli_app_loop;
    ext.on_wifi_connected = wscli_app_on_wifi_connected;
    ext.on_wifi_disconnected = wscli_app_on_wifi_disconnected;
    ext.on_websock_connected = wscli_app_on_websock_connected;
    ext.on_websock_disconnected = wscli_app_on_websock_disconnected;
    ext.on_websock_data = wscli_app_on_websock_data;
    ext.on_websock_error = wscli_app_on_websock_error;
    ext.on_websock_loop = wscli_app_on_websock_loop;
    wscli_app_main(&ext);

    fflush(stdout);
    delay(1000);
    esp_restart();
}
