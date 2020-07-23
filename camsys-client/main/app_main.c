#include <stdio.h>
#include "sdkconfig.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"


#define CAMSYS_NAMESPACE "camsys"

static const char *TAG = "camsys";

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
    void* card;
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
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    struct mount_config_s {
        bool format_if_mount_failed;
        int max_files;
        size_t allocation_unit_size;
    } mount_config;
    mount_config.format_if_mount_failed = auto_format;
    mount_config.max_files = max_files;
    mount_config.allocation_unit_size = alloc_unit_size;
    //};
    
    const char mount_point[] = SDCARD_MOUNT_POINT;
    // ESP_LOGI("sdcard", "Initializing SD card");

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
    // ESP_LOGI("sdcard", "Using SPI peripheral");

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
        // ESP_LOGE("sdcard", "Failed to initialize bus.");
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
            // ESP_LOGE("sdcard", "Failed to mount filesystem. "
            //     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            // ESP_LOGE("sdcard", "Failed to initialize the card (%s). "
            //     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
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
    // ESP_LOGE("sdcard", "Card unmounted");
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

esp_err_t wifi_creds_save(wifi_creds_t creds, const char* namespace) {
    // Open
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace && namespace[0] ? namespace : MYESP_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    // Write
    err = nvs_set_blob(handle, "wifi_creds", creds, WIFI_CREDS_SIZE);
    if (err == ESP_OK) err = nvs_commit(handle);

    // Close
    nvs_close(handle);
    
    return err;
}

esp_err_t wifi_creds_load(wifi_creds_t creds, const char* namespace) {
    wifi_creds_clear(creds);
    // // WIFI_CREDS_SET(creds, 0, "apucika", "mad12345");
    // // WIFI_CREDS_SET(creds, 1, "apucika_EXT", "mad12345");
    // WIFI_CREDS_SET(creds, 2, "Ulefone_S1", "dde33cc5ed6c");

    // Open
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace && namespace[0] ? namespace : MYESP_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    // Read
    size_t length = WIFI_CREDS_SIZE;
    err = nvs_get_blob(handle, "wifi_creds", creds, &length);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        wifi_creds_clear(creds);
        length = WIFI_CREDS_SIZE;
        err = ESP_OK;
    }
    if (err == ESP_OK) if (length != WIFI_CREDS_SIZE) err = ESP_FAIL;

    // Close
    nvs_close(handle);
    
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

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "WIFI EVENT:%d", event_id);
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
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_retry_num = 0;
        wifi_disconnected = false;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_event_handler_instance_t wifi_handle_instance_disconnect;


static void wifi_disconnect_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) 
{
    ESP_LOGI(TAG, "WIFI DISCONNECT EVENT HANDLER:%d, TODO: reconnect...", event_id);
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

    ESP_LOGI(TAG, "wifi_init_sta finished.");

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
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 ssid, pswd);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 ssid, pswd);
        ret = ESP_ERR_WIFI_NOT_CONNECT;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        ret = ESP_FAIL;
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(wifi_event_group);
    return ret;
}

esp_err_t wifi_scan_connect(wifi_creds_t creds) {
    wifi_retry_num = 0;

    wifi_scan_list_t ap_info;
    uint16_t ap_count = wifi_scan(ap_info);

    for (int i = 0; (i < WIFI_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        // ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
        // //print_auth_mode(ap_info[i].authmode);
        // if (ap_info[i].authmode != WIFI_AUTH_WEP) {
        //     //print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        // }
        // ESP_LOGI(TAG, "Channel \t\t%d\n", ap_info[i].primary);

        for (int j=0; j<WIFI_CREDENTIALS; j++) {
            if (WIFI_CREDS_GET_SSID(creds, j)[0] && !strcmp(WIFI_CREDS_GET_SSID(creds, j), (char*)ap_info[i].ssid)) {
                ESP_LOGI(TAG, "cred found at (%d):'%s':'%s' => '%s'\n", i, 
                    WIFI_CREDS_GET_SSID(creds, j), 
                    WIFI_CREDS_GET_PSWD(creds, j), 
                    wifi_creds_get_pswd(creds,WIFI_CREDS_GET_SSID(creds, j))
                );
                if (ESP_OK == wifi_connect(WIFI_CREDS_GET_SSID(creds, j), WIFI_CREDS_GET_PSWD(creds, j))) return ESP_OK;
            }
        }
    }
    return ESP_ERR_WIFI_NOT_CONNECT;
}

typedef void (*wifi_app_cb_func_t)(void);

void wifi_connection_establish(wifi_creds_t creds, wifi_app_cb_func_t loop) {
    while(ESP_OK != wifi_scan_connect(creds)) {
        loop();
        ESP_LOGI(TAG, "WIFI connection failed. retry...\n");   
    }
    ESP_LOGI(TAG, "WIFI is now connected.\n"); 
}

void wifi_connection_keep_alive(wifi_creds_t creds, wifi_app_cb_func_t loop) {
    if (wifi_disconnected) wifi_connection_establish(creds, loop);
}


bool wifi_app_exited = false;

void wifi_app_run(wifi_creds_t creds, wifi_app_cb_func_t loop_while_connecting, wifi_app_cb_func_t loop_with_connection) {
    wifi_init();

    wifi_connection_establish(creds, loop_while_connecting);

    while(!wifi_app_exited) {
        wifi_connection_keep_alive(creds, loop_while_connecting);
        loop_with_connection();
    }
    ESP_LOGI(TAG, "\nOK, LEAVING...");
}

#define WIFI_APP_MODE_SETUP 0
#define WIFI_APP_MODE_RUN 1

int wifi_app_get_mode() {
    gpio_pins_init(1ULL<<GPIO_NUM_13);
    return gpio_get_level(GPIO_NUM_13) ? WIFI_APP_MODE_RUN : WIFI_APP_MODE_SETUP;
}

// --------------

/*

100 KEY
KF4GTX9
200 OK
100 SSID 10/1:
apucika
200 OK
100 PSWD 10/1:
mad12345
200 OK
100 SSID 10/2:
apucika_EXT
200 OK
100 PSWD 10/2:
mad12345
200 OK
100 SSID 10/3:

200 OK
100 PSWD 10/3:

....

200 OK Credentials are saved.

*/

// TODO: Unique code for each device. Should be changed before flash!
#define WIFI_APP_SETTINGS_KEY "KF4GTX9"

void wifi_app_setup(wifi_app_cb_func_t setup) {
    ESP_ERROR_CHECK( serial_install(UART_NUM_0, ESP_LINE_ENDINGS_CR, ESP_LINE_ENDINGS_CRLF) );

    printf("100 KEY\n");
    char* key = serial_readln();
    if (!strcmp(key, WIFI_APP_SETTINGS_KEY)) {
        free(key);

        wifi_creds_t creds;
        wifi_creds_clear(creds);

        for (int i=1; i<=WIFI_CREDENTIALS; i++) {
            printf("200 OK\n");

            printf("100 SSID %d/%d:\n", WIFI_CREDENTIALS, i);
            char* ssid = serial_readln();
            printf("200 OK\n");

            printf("100 PSWD %d/%d:\n", WIFI_CREDENTIALS, i);
            char* pswd = serial_readln();
            WIFI_CREDS_SET(creds, i, ssid, pswd);
            free(ssid);
            free(pswd);            
        }

        esp_err_t err = wifi_creds_save(creds, "camsys");
        ESP_OK == err ?             
            printf("200 OK Credentials are saved.\n") :
            printf("300 ERROR Saving credential error: %s\n", esp_err_to_name(err));

    } else {
        free(key);
        printf("300 ERROR\n");
    }

    ESP_ERROR_CHECK( serial_uninstall(UART_NUM_0) );
}

// --------------

void app_setup() {
    ESP_LOGI(TAG, "App setup when wifi setting just readed..\n");
    printf("app setup..\n");
    delay(1000);
}

void app_loop_while_connecting() {
    ESP_LOGI(TAG, "App working when no wifi..\n");
    printf("no wifi..\n");
    delay(1000);
}

void app_loop_with_connection() {
    ESP_LOGI(TAG, "App working when wifi connected..\n");
    printf("has wifi..\n");
    delay(1000);
}

void app_main(void)
{
    //esp_log_level_set("*", ESP_LOG_NONE);

    flash_init();
    
    wifi_creds_t creds;
    int mode = wifi_app_get_mode();
    switch (mode) {
        case WIFI_APP_MODE_SETUP:
            wifi_app_setup(app_setup);
            break;
        case WIFI_APP_MODE_RUN:

            ESP_ERROR_CHECK( wifi_creds_load(creds, "camsys") );

            wifi_app_run(creds, app_loop_while_connecting, app_loop_with_connection);
            break;
        default:
            ESP_LOGE(TAG, "Incorrect Wifi App Mode: %d", mode);
    }

    fflush(stdout);
    delay(1000);
    esp_restart();
}
