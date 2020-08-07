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
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "hal/uart_types.h"
#include "driver/uart.h"
#include "driver/sdmmc_types.h"
#include "driver/sdspi_host.h"

#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"

// ------------------------- CAMERA INCLUDES --------------------------
#define CONFIG_OV2640_SUPPORT true
#include "esp_camera.h"
#include "xclk.h"
#include "sccb.h"
#include "ov2640.h"
#include "sensor.h"
#include "yuv.h"
#include "twi.h"
#include "esp_jpg_decode.h"

#define TAG "espcam"
#include "xclk.c"
#include "ov2640.c"
#include "sccb.c"
#include "sensor.c"
#include "yuv.c"
#include "twi.c"
#include "to_bmp.c"
#include "esp_jpg_decode.c"
#include "camera.c"
#undef TAG
// --------------------------------------------------------------------




#define CAMSYS_NAMESPACE "camsys"

static const char *TAG = CAMSYS_NAMESPACE;

// ---------------------------------------------------------------
// DELAY
// ---------------------------------------------------------------

void delay(long ms) {
    vTaskDelay(ms / portTICK_RATE_MS);
}

/*
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
*/

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
#undef SDCARD_TARGET_ESP32

#ifdef SDCARD_TARGET_ESP32
#include "driver/sdmmc_host.h"
#endif

#define SDCARD_MOUNT_POINT "/sdcard"
#define SDCARD_FORMAT_IF_MOUNT_FAILED false

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
#define SDCARD_SPI_DMA_CHAN    sdcard_host.slot
#endif //SDCARD_TARGET_ESP32S2

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


const char sdcard_mount_point[] = SDCARD_MOUNT_POINT;
sdmmc_card_t* sdcard;

#ifndef SDCARD_USE_SPI_MODE
sdmmc_host_t sdcard_host = SDMMC_HOST_DEFAULT();
#else
sdmmc_host_t sdcard_host = SDSPI_HOST_DEFAULT();
#endif //SDCARD_USE_SPI_MODE

bool sdcard_initialized = false;

esp_err_t sdcard_init()
{
    esp_err_t ret = ESP_OK;
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = SDCARD_FORMAT_IF_MOUNT_FAILED,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
#ifndef SDCARD_USE_SPI_MODE
    ESP_LOGI(TAG, "Using SDMMC peripheral");

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

    ret = esp_vfs_fat_sdmmc_mount(sdcard_mount_point, &sdcard_host, &slot_config, &mount_config, &sdcard);
#else
    ESP_LOGI(TAG, "Using SPI peripheral");

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SDCARD_PIN_NUM_MOSI,
        .miso_io_num = SDCARD_PIN_NUM_MISO,
        .sclk_io_num = SDCARD_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(sdcard_host.slot, &bus_cfg, SDCARD_SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ret;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SDCARD_PIN_NUM_CS;
    slot_config.host_id = sdcard_host.slot;

    ret = esp_vfs_fat_sdspi_mount(sdcard_mount_point, &sdcard_host, &slot_config, &mount_config, &sdcard);
#endif //SDCARD_USE_SPI_MODE

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                "If you want the card to be formatted, set the SDCARD_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return ret;
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, sdcard);

    sdcard_initialized = (ret == ESP_OK);
    return ret;
}

void sdcard_close() {
    if (!sdcard_initialized) return;
    // All done, unmount partition and disable SDMMC or SPI peripheral
    esp_vfs_fat_sdcard_unmount(sdcard_mount_point, sdcard);
    ESP_LOGI(TAG, "Card unmounted");
#ifdef SDCARD_USE_SPI_MODE
    //deinitialize the bus after all devices are removed
    spi_bus_free(sdcard_host.slot);
#endif
}

// ---------------------------------------------------------------
// STRING EXTRAS
// ---------------------------------------------------------------

bool str_starts_with(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? false : memcmp(pre, str, lenpre) == 0;
}

char* strncpy_offs(char* dest, size_t offs, const char* src, size_t max) {
    int i=0;
    for (; i+offs<max; i++) {
        if (!src[i]) break;
        dest[i+offs] = src[i];
    }
    dest[i+offs] = '\0';
    return dest;
}

// ------------------------------------------------------
// CAMERA
// ------------------------------------------------------

struct camsys_camera_s {
    FILE* file;
};

typedef struct camsys_camera_s camsys_camera_t;

void camera_recording_init(camsys_camera_t* camera) {
    camera->file = NULL;
}

esp_err_t camera_recording_stop(camsys_camera_t* camera) {
    if (fclose(camera->file)) return ESP_FAIL;
    camera->file = NULL;
    return ESP_OK;
}


esp_err_t camera_recording_open(camsys_camera_t* camera, const char* mode) {
    if (camera->file && ESP_OK != camera_recording_stop(camera)) return ESP_FAIL;
    
    camera->file = fopen(SDCARD_MOUNT_POINT"/record", mode);
    if (!camera->file) {
        ESP_LOGE(TAG, "file open error: %d", errno);
        return ESP_FAIL;
    }
    return ESP_OK;  
}

esp_err_t camera_recording_start(camsys_camera_t* camera) {
    return camera_recording_open(camera, "ab");
}

esp_err_t camera_recording_delete(camsys_camera_t* camera) {
    esp_err_t err_open = camera_recording_open(camera, "w+");
    esp_err_t err_stop = camera_recording_stop(camera);
    return err_open ? err_open : err_stop;
}

// ------------------------------------------------------
// MOTION
// ------------------------------------------------------

struct camsys_motion_s {
    
};

typedef struct camsys_motion_s camsys_motion_t;

// ------------------------------------------------------
// CAMSYS
// ------------------------------------------------------

#define CAMSYS_MODE_CAMERA true
#define CAMSYS_MODE_MOTION false

// do different things when mode is camera/motion
#define CAMSYS_BY_APP_MODE(camera, motion) if (app->ext->sys->mode) camera else motion
#define CAMSYS_BY_EXT_MODE(camera, motion) if (ext->sys->mode) camera else motion
#define CAMSYS_BY_SYS_MODE(camera, motion) if (sys->mode) camera else motion
#define CAMSYS_BY_MODE(camera, motion) if (mode) camera else motion



struct camsys_s {
    bool mode;
    bool streaming;
    camsys_camera_t* camera;
    camsys_motion_t* motion;

    httpd_handle_t server;
};

typedef struct camsys_s camsys_t;

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

#define WSCLI_APP_SECRET_SIZE 33
typedef char wscli_app_secret_t[WSCLI_APP_SECRET_SIZE];
#define WSCLI_APP_CID_SIZE 33
typedef char wscli_app_cid_t[WSCLI_APP_CID_SIZE];

struct wscli_app_s {

    int no_data_timeout_sec;
    const char* wsuri_fmt;
    esp_websocket_client_config_t websocket_cfg;
    esp_websocket_client_handle_t client;

    app_cb_func_t settings_func;
    app_cb_func_t loop_func;

    app_cb_func_t on_wifi_connected;
    app_cb_func_t on_wifi_disconnected;
    app_cb_func_t on_websock_connected;
    app_cb_func_t on_websock_disconnected;
    websock_app_cb_func_t on_websock_data;
    websock_app_cb_func_t on_websock_error;
    app_cb_func_t on_websock_loop;

    wscli_app_secret_t secret;
    wscli_app_cid_t cid;

    camsys_t* sys;
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
// CAMSYS APP
// ---------------------------------------------------------------


//CAMERA_MODEL_AI_THINKER PIN Map
#define CAM_PIN_PWDN    32 //power down is not used
#define CAM_PIN_RESET   -1 //software reset will be performed
#define CAM_PIN_XCLK    0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27

#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

#define CAMSYS_CAMERA_CONFIG_DEFAULT() { \
    .pin_pwdn  = CAM_PIN_PWDN, \
    .pin_reset = CAM_PIN_RESET, \
    .pin_xclk = CAM_PIN_XCLK, \
    .pin_sscb_sda = CAM_PIN_SIOD, \
    .pin_sscb_scl = CAM_PIN_SIOC, \
    .pin_d7 = CAM_PIN_D7, \
    .pin_d6 = CAM_PIN_D6, \
    .pin_d5 = CAM_PIN_D5, \
    .pin_d4 = CAM_PIN_D4, \
    .pin_d3 = CAM_PIN_D3, \
    .pin_d2 = CAM_PIN_D2, \
    .pin_d1 = CAM_PIN_D1, \
    .pin_d0 = CAM_PIN_D0, \
    .pin_vsync = CAM_PIN_VSYNC, \
    .pin_href = CAM_PIN_HREF, \
    .pin_pclk = CAM_PIN_PCLK, \
    .xclk_freq_hz = 20000000, \
    .ledc_timer = LEDC_TIMER_0, \
    .ledc_channel = LEDC_CHANNEL_0, \
    .pixel_format = PIXFORMAT_JPEG, \
    .frame_size = FRAMESIZE_QVGA, \
    .jpeg_quality = 12, \
    .fb_count = 1 \
};

void camsys_camera_init(bool mode, bool power_up) {
    //power up the camera if PWDN pin is defined
    if(power_up && CAM_PIN_PWDN != -1){
        pinMode(CAM_PIN_PWDN, OUTPUT);
        digitalWrite(CAM_PIN_PWDN, LOW);
    }

    camera_config_t config = CAMSYS_CAMERA_CONFIG_DEFAULT();
    CAMSYS_BY_MODE({
        config.pixel_format = PIXFORMAT_JPEG;
        config.frame_size = FRAMESIZE_QVGA;
        config.jpeg_quality = 12;
    }, {
        config.pixel_format = PIXFORMAT_GRAYSCALE;
        config.frame_size = FRAMESIZE_96X96;
        config.jpeg_quality = 0;
    });    

    //initialize the camera
    ESP_ERROR_CHECK( esp_camera_init(&config) );
}

// --- nvs ---

esp_err_t camsys_app_mode_save(nvs_handle_t handle, uint8_t mode) {
    ESP_LOGE(TAG, "mode set to %d", mode);
    esp_err_t err = nvs_set_u8(handle, "mode", mode);
    if (err == ESP_OK) err = nvs_commit(handle);
    return err;
}

esp_err_t camsys_app_mode_load(nvs_handle_t handle, uint8_t* mode) {
    esp_err_t err = nvs_get_u8(handle, "mode", mode);
    if (err == ESP_ERR_NVS_NOT_FOUND) err = ESP_OK;
    return err;
}


// ---------------------------------------------------------------
// CAMSYS APP (streaming)
// ---------------------------------------------------------------

#define CAMERA_PART_BOUNDARY "123456789000000000000987654321"
static const char* CAMSYS_CAMERA_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" CAMERA_PART_BOUNDARY;
static const char* CAMSYS_CAMERA_STREAM_BOUNDARY = "\r\n--" CAMERA_PART_BOUNDARY "\r\n";
static const char* CAMSYS_CAMERA_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

wifi_app_t* _app = NULL;

bool camsys_fb_hold = false;

camera_fb_t * camsys_fb_get(wifi_app_t* app) {
    while(camsys_fb_hold);
    camsys_fb_hold = true;
    camera_fb_t * fb = esp_camera_fb_get();
    if (fb && app->ext->sys->mode == CAMSYS_MODE_CAMERA && app->ext->sys->camera->file) {
        ESP_LOGI(TAG, "write frame structure..");
        size_t written = fwrite(fb, sizeof(camera_fb_t), 1, app->ext->sys->camera->file);
        if (written != 1) {
            ESP_LOGE(TAG, "write error: written: %d != 1, error: %d", written, ferror(app->ext->sys->camera->file));
        }
        else {
            ESP_LOGI(TAG, "write frame buffer..");
            written = fwrite(fb->buf, sizeof(uint8_t), fb->len, app->ext->sys->camera->file);
            if (written != fb->len) {
                ESP_LOGE(TAG, "write error: written: %d != fb->len: %d, error: %d", written, fb->len, ferror(app->ext->sys->camera->file));
            }
        }
    }
    return fb;
}

esp_err_t camsys_fb_return(camera_fb_t * fb) {
    if(!camsys_fb_hold) return ESP_FAIL;
    esp_camera_fb_return(fb);
    camsys_fb_hold = false;
    return ESP_OK;
}

bool camsys_check_secret(wifi_app_t* app, httpd_req_t* req) {
    //return true; // TODO: remove this line to validate secret parameter
    bool secret_ok = false;
    if (!app->ext->secret || app->ext->secret[0] == '\0') return secret_ok;

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    size_t buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        char* buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI("camsys", "Found URL query"/*, buf*/);
            char param[40];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "secret", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI("camsys", "Found URL query parameter => secret"/*, param*/);
                if (!strcmp(param, app->ext->secret)) secret_ok = true;
            }
        }
        free(buf);
    }
    return secret_ok;
}

camera_fb_t * replay_fb_get(wifi_app_t* app) {
    camera_fb_t* fb = malloc(sizeof(camera_fb_t));
    if (!fb) {
        ESP_LOGE(TAG, "mem alloc for fb error");
        return NULL;
    }
    FILE* f = app->ext->sys->camera->file;
    if (!f) {
        ESP_LOGE(TAG, "record file is not open");
        free(fb);
        return NULL;
    }
    if (1 != fread(fb, sizeof(camera_fb_t), 1, f)) {
        ESP_LOGE(TAG, "record fb read error");
        free(fb);
        return NULL;
    }
    fb->buf = malloc(fb->len);
    if (!fb) {
        ESP_LOGE(TAG, "mem alloc for fb->buf error");
        free(fb);
        return NULL;
    }
    if (fb->len != fread(fb->buf, sizeof(uint8_t), fb->len, f)) {
        ESP_LOGE(TAG, "record fb->buf read error");
        free(fb->buf);
        free(fb);
        return NULL;
    }

    return fb;
}

esp_err_t replay_fb_return(camera_fb_t* fb) {
    free(fb->buf);
    free(fb);
    return ESP_OK;
}

esp_err_t camsys_camera_httpd_stream_replay_handler(wifi_app_t* app, httpd_req_t* req, bool replay) {
    esp_err_t res = ESP_OK;
    camera_fb_t * fb = NULL;
    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;
    char * part_buf[64];

    res = httpd_resp_set_type(req, CAMSYS_CAMERA_STREAM_CONTENT_TYPE);
    if(res != ESP_OK) return res;
    
    //ESP_ERROR_CHECK_WITHOUT_ABORT( camera_recording_start(app->ext->sys->camera) ); // TODO: remove this to switch off recording autostart

    app->ext->sys->streaming = true;

    if (replay) ESP_ERROR_CHECK( camera_recording_open(app->ext->sys->camera, "rb") );

    while(app->ext->sys->streaming) {   
        
        fb = replay ? replay_fb_get(app) : camsys_fb_get(app);
        
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }

        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
        

        if(res == ESP_OK) {
            res = httpd_resp_send_chunk(req, CAMSYS_CAMERA_STREAM_BOUNDARY, strlen(CAMSYS_CAMERA_STREAM_BOUNDARY));
        }
        if(res == ESP_OK) {
            size_t hlen = snprintf((char *)part_buf, 64, CAMSYS_CAMERA_STREAM_PART, _jpg_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb->format != PIXFORMAT_JPEG) {
            free(_jpg_buf);
        }
        if(res != ESP_OK || (replay ? replay_fb_return(fb) : camsys_fb_return(fb)) != ESP_OK) {
            if (res == ESP_OK) res = ESP_FAIL;
            break;
        }
        
    }

    if (replay) ESP_ERROR_CHECK( camera_recording_stop(app->ext->sys->camera) );
    
    //ESP_ERROR_CHECK_WITHOUT_ABORT( camera_recording_stop(app->ext->sys->camera) );// TODO: remove this when recording autostart is switched off 
    
    app->ext->sys->streaming = false;

    return res;
}


#define MOTION_PART_BOUNDARY "123456789000000000000987654321"
static const char* MOTION__STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" MOTION_PART_BOUNDARY;
static const char* MOTION__STREAM_BOUNDARY = "\r\n--" MOTION_PART_BOUNDARY "\r\n";
static const char* MOTION__STREAM_PART = "Content-Type: image/x-windows-bmp\r\nContent-Length: %u\r\n\r\n";

camera_fb_t* motion_fb = NULL;

esp_err_t camsys_motion_httpd_image_handler(wifi_app_t* app, httpd_req_t* req) {
    camera_fb_t * fb = motion_fb;
    esp_err_t res = ESP_OK;
    size_t _bmp_buf_len;
    uint8_t * _bmp_buf;
    char * part_buf[64];

    res = httpd_resp_set_type(req, MOTION__STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    app->ext->sys->streaming = true;

    while(app->ext->sys->streaming) {
        if (!fb) {
            fb = esp_camera_fb_get();
        }
        if (!fb) {
            ESP_LOGE("camsys", "Camera capture failed");
            res = ESP_FAIL;
            break;
        }
        
        bool bmp_converted = frame2bmp(fb, &_bmp_buf, &_bmp_buf_len);
        if(!bmp_converted){
            ESP_LOGE("camsys", "BMP compression failed");
            esp_camera_fb_return(fb);
            res = ESP_FAIL;
        }


        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, MOTION__STREAM_BOUNDARY, strlen(MOTION__STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, MOTION__STREAM_PART, _bmp_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_bmp_buf, _bmp_buf_len);
        }
        if(fb->format != PIXFORMAT_JPEG){
            free(_bmp_buf);
        }
        // esp_camera_fb_return(fb);
        if(res != ESP_OK){
            break;
        }
    }

    app->ext->sys->streaming = false;

    return res;
}

esp_err_t camsys_httpd_stream_replay_handler(httpd_req_t* req, bool replay) {
    wifi_app_t* app = _app;
    if (!camsys_check_secret(app, req)) return ESP_FAIL;
    if (app->ext->sys->mode == CAMSYS_MODE_CAMERA) return camsys_camera_httpd_stream_replay_handler(app, req, replay);
    if (app->ext->sys->mode == CAMSYS_MODE_MOTION) return camsys_motion_httpd_image_handler(app, req);
    ESP_ERROR_CHECK( httpd_resp_send_404(req) );
    return ESP_OK;
}

esp_err_t camsys_httpd_stream_handler(httpd_req_t* req) {
    return camsys_httpd_stream_replay_handler(req, false);
}

esp_err_t camsys_httpd_replay_handler(httpd_req_t* req) {
    return camsys_httpd_stream_replay_handler(req, true);
}


static const httpd_uri_t camsys_stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = camsys_httpd_stream_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

static const httpd_uri_t camsys_record_uri = {
    .uri       = "/replay",
    .method    = HTTP_GET,
    .handler   = camsys_httpd_replay_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};


//Function for starting the webserver
void camsys_httpd_server_init(wifi_app_t* app)
{
    _app = app;
    // Generate default configuration
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Empty handle to http_server
    app->ext->sys->server = NULL;

    // Start the httpd server
    ESP_ERROR_CHECK(httpd_start(&app->ext->sys->server, &config));
    // Register URI handlers
    ESP_ERROR_CHECK(httpd_register_uri_handler(app->ext->sys->server, &camsys_stream_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(app->ext->sys->server, &camsys_record_uri));

    // If server failed to start, handle will be NULL
}


// -------------------------

#define WEBSOCK_SEND_BUFF_SIZE 1000
char websock_send_buff[WEBSOCK_SEND_BUFF_SIZE];

esp_err_t websock_sendf(esp_websocket_client_handle_t client, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int slen = vsnprintf(websock_send_buff, WEBSOCK_SEND_BUFF_SIZE, fmt, args);
    //ESP_LOGI(TAG, "sending: [%s]..", websock_send_buff);
    if (slen < 0 || slen != esp_websocket_client_send_text(client, websock_send_buff, slen, portMAX_DELAY)) return ESP_FAIL;
    va_end(args);
    return ESP_OK;
}


// ----------------- camera/motion websocket loops ---------------------

void camsys_camera_websock_loop(wifi_app_t* app) {
    // TODO ...
    
}

// -----------------------------------

#define WATCHER_MAX_SIZE 40
#define WATCHER_BUFF_SIZE WATCHER_MAX_SIZE*WATCHER_MAX_SIZE*4
uint8_t watcher_prev_buf[WATCHER_BUFF_SIZE];

struct watcher_s {
    int x;
    int y;
    int size;
    int raster;
    int threshold;

    size_t diff_sum_max;
};

typedef struct watcher_s watcher_t;

#define WATCHER_DEFAULT {43, 43, 10, 5, 250, 0}

watcher_t watcher = WATCHER_DEFAULT;


void watcher_show_diff(size_t diff_sum, bool alert) {
    char spc[] = "]]]]]]]]]]]]]]]]]]]]]]]]]]]]][";
    char* s = spc; 
    for (int i=0; i<29; i++) {
        if (i*10>diff_sum) s[i] = ' ';
    }
    char sbuff[100];
    snprintf(sbuff, 100, "%s (%u) %s", s, diff_sum, alert ? "ALERT!!!" : "");
    ESP_LOGI(TAG, "%s", sbuff);
}

bool camsys_motion_websock_loop_first = true;
bool camsys_motion_websock_loop_alert = false;
void camsys_motion_websock_loop(wifi_app_t* app) {
    // TODO ...
    camera_fb_t* fb = motion_fb;

    fb = camsys_fb_get(app);            
    if (!fb) {
        ESP_LOGE(TAG, "Motion Camera capture failed");
        ESP_ERROR_CHECK( camsys_fb_return(fb) );
        return;
    }
    

    int xfrom = watcher.x - watcher.size;
    int xto = watcher.x + watcher.size;
    int yfrom = watcher.y - watcher.size;
    int yto = watcher.y + watcher.size;
    int i=0;
    size_t diff_sum = 0;
    for (int x=xfrom; x<xto; x+=watcher.raster) {
        for (int y=yfrom; y<yto; y+=watcher.raster) {
            int diff = fb->buf[x+y*fb->width] - watcher_prev_buf[i];
            diff_sum += (diff > 0 ? diff : -diff);
            if (i>=WATCHER_BUFF_SIZE) {
                ESP_LOGE(TAG, "buff size too large");
                break;
            }
            watcher_prev_buf[i] = fb->buf[x+y*fb->width];
            i++;
        }
    }

    if (watcher.diff_sum_max < diff_sum) watcher.diff_sum_max = diff_sum;
    
    if (!camsys_motion_websock_loop_alert) 
        camsys_motion_websock_loop_alert = !camsys_motion_websock_loop_first && diff_sum >= watcher.threshold;
    camsys_motion_websock_loop_first = false;

    watcher_show_diff(diff_sum, camsys_motion_websock_loop_alert);
    if (camsys_motion_websock_loop_alert) {
        // PRINT("******************************************************");
        // PRINT("*********************** [ALERT] **********************");
        // PRINT("******************************************************");
        if (ESP_OK == websock_sendf(app->ext->client, "{\"func\":\"alert\"}") ) camsys_motion_websock_loop_alert = false;
    }
    
    ESP_ERROR_CHECK( camsys_fb_return(fb) );
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

    // TODO may cam should be initialized here?

    wifi_init();

    camsys_httpd_server_init(app);

    // TODO may cam should be initialized here?
    sdcard_init();

    wifi_connection_establish(app);

    // TODO may cam should be initialized here?

    while(!app->exited) {
        wifi_connection_keep_alive(app);
        app->ext->loop_func(app);
    }

    sdcard_close();
}

#define WIFI_APP_MODE_SETTING 0
#define WIFI_APP_MODE_RUN 1

int wifi_app_get_mode() {
    gpio_pins_init(1ULL<<GPIO_NUM_13);
    return gpio_get_level(GPIO_NUM_13) ? WIFI_APP_MODE_RUN : WIFI_APP_MODE_SETTING;
}

// --------------


// TODO: Unique code for each device. Should be changed before flash!
#define WIFI_APP_SETTINGS_KEY "KF4GTX9"


void wifi_app_settings(wifi_app_t* app) {
    ESP_ERROR_CHECK( serial_install(UART_NUM_0, ESP_LINE_ENDINGS_CR, ESP_LINE_ENDINGS_CRLF) );

    char * in;
    while(true) {
        printf("100 HELLO\n");
        in = serial_readln();
        if (!strcmp(in, "HELLO")) break;
        free(in);
        printf("300 ERROR Type HELLO\n");
    }
    free(in);
    printf("200 OK\n");

    printf("100 KEY\n");
    char* key = serial_readln();
    if (!strcmp(key, WIFI_APP_SETTINGS_KEY)) {
        free(key);

        wifi_creds_clear(app->wifi_creds);

        for (int i=1; i<=WIFI_CREDENTIALS; i++) {
            printf("200 OK\n");

            printf("100 SSID %d/%d\n", WIFI_CREDENTIALS, i);
            char* ssid = serial_readln();
            printf("200 OK\n");

            printf("100 PSWD %d/%d\n", WIFI_CREDENTIALS, i);
            char* pswd = serial_readln();
            WIFI_CREDS_SET(app->wifi_creds, i, ssid, pswd);
            free(ssid);
            free(pswd);            
        }

        esp_err_t err = wifi_creds_save(app);
        ESP_OK == err ?             
            printf("200 OK\n") :
            printf("300 ERROR Saving credential error: %s\n", esp_err_to_name(err));

        if (app->ext->settings_func) app->ext->settings_func(app);
    } else {
        free(key);
        printf("300 ERROR Key missmatch\n");
    }

    printf("300 Setup process finished\n");

    ESP_ERROR_CHECK( serial_uninstall(UART_NUM_0) );
}



void wifi_app_main(wifi_app_t* app) {


    // TODO may cam should be initialized here? 1
    flash_init();

    // TODO may cam should be initialized here?

    // NVS Open
    ESP_ERROR_CHECK( nvs_open(app->namespace, NVS_READWRITE, &app->nvs_handle) );
    
    // TODO may cam should be initialized here?

    app->mode = wifi_app_get_mode();

    // TODO may cam should be initialized here?

    uint8_t mode = CAMSYS_MODE_MOTION;
    switch (app->mode) {
        case WIFI_APP_MODE_SETTING:
            wifi_app_settings(app);
            delay(2000);
            break;
        case WIFI_APP_MODE_RUN:
            ESP_ERROR_CHECK( camsys_app_mode_load(app->nvs_handle, &mode) );
            app->ext->sys->mode = (bool)mode;

            ESP_LOGI(TAG, "mode load from settings: %d", (uint8_t)app->ext->sys->mode);

            // TODO may cam should be initialized here?
            camsys_camera_init(app->ext->sys->mode, true);
            ESP_LOGI(TAG, "camera initialized...");

            ESP_ERROR_CHECK( wifi_creds_load(app) );

            // TODO may cam should be initialized here?

            wifi_app_run(app);
            break;
        default:
            ESP_LOGE(TAG, "Incorrect Wifi App Mode: %d", app->mode);
            app->exited = true;
            break;
    }

    // NVS Close
    nvs_close(app->nvs_handle);
}

// ---------------------------------------------------------------
// WEBSOCKET CLIENT APP (settings)
// ---------------------------------------------------------------


esp_err_t wscli_app_host_secret_cid_save(nvs_handle_t handle, const char* host, const char* secret, const char* cid) {
    // Write
    esp_err_t err = nvs_set_str(handle, "host", host);
    if (err == ESP_OK) err = nvs_set_str(handle, "secret", secret);
    if (err == ESP_OK) err = nvs_set_str(handle, "cid", cid);
    if (err == ESP_OK) err = nvs_commit(handle);
    
    return err;
}

esp_err_t wscli_app_host_secret_cid_load(nvs_handle_t handle, char* host, size_t host_length, char* secret, size_t secret_length, char* cid, size_t cid_length) {
    size_t required_size;
    esp_err_t err = nvs_get_str(handle, "host", NULL, &required_size);
    if (err == ESP_OK) {
        if (required_size >= host_length) return ESP_ERR_NVS_INVALID_LENGTH;
        err = nvs_get_str(handle, "host", host, &required_size);
    }
    if (err == ESP_OK) err = nvs_get_str(handle, "secret", NULL, &required_size);
    if (err == ESP_OK) {
        if (required_size >= secret_length) return ESP_ERR_NVS_INVALID_LENGTH;
        err = nvs_get_str(handle, "secret", secret, &required_size);
    }
    if (err == ESP_OK) err = nvs_get_str(handle, "cid", NULL, &required_size);
    if (err == ESP_OK) {
        if (required_size >= cid_length) return ESP_ERR_NVS_INVALID_LENGTH;
        err = nvs_get_str(handle, "cid", cid, &required_size);
    }
    return err;
}

void wscli_app_settings(void* arg) {
    wifi_app_t* app = arg;
    printf("100 HOST\n");
    char* host = serial_readln();
    printf("200 OK\n");
    
    printf("100 SECRET\n");
    char* secret = serial_readln();
    printf("200 OK\n");
    
    printf("100 CID\n");
    char* cid = serial_readln();
    
    esp_err_t err = wscli_app_host_secret_cid_save(app->nvs_handle, host, secret, cid);
    ESP_OK == err ?             
        printf("200 OK\n") :
        printf("300 ERROR host/secret write error: %s\n", esp_err_to_name(err));        

    free(host);
    free(secret);
    free(cid);


    printf("100 MODE\n");
    char* modestr = serial_readln();

    err = camsys_app_mode_save(app->nvs_handle, (uint8_t)(!strcmp(modestr, "camera") ? CAMSYS_MODE_CAMERA : CAMSYS_MODE_MOTION));
    
    ESP_OK == err ?             
        printf("200 OK\n") :
        printf("300 ERROR device mode write error: %s\n", esp_err_to_name(err));    

    free(modestr);    
}

// ---------------------------------------------------------------
// WEBSOCKET CLIENT APP (loop)
// ---------------------------------------------------------------


static TimerHandle_t shutdown_signal_timer;
static SemaphoreHandle_t shutdown_sema;
// #define SHOTDOWN_COUNTER_START 3
// int shutdown_counter = SHOTDOWN_COUNTER_START;

static void wscli_app_shutdown_signaler(TimerHandle_t xTimer)
{
//     ESP_LOGI(TAG, "shutdown count: %d", shutdown_counter--);
//     if (!shutdown_counter) esp_restart();
    xSemaphoreGive(shutdown_sema);
}

static void wscli_app_websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    wifi_app_t* app = handler_args;
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
//         shutdown_counter = SHOTDOWN_COUNTER_START;
        if (app->ext->on_websock_connected) (app->ext->on_websock_connected)(app);
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        if (app->ext->on_websock_disconnected) (app->ext->on_websock_disconnected)(app);
        break;
    case WEBSOCKET_EVENT_DATA:
        xTimerReset(shutdown_signal_timer, portMAX_DELAY);

//         ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
//         ESP_LOGI(TAG, "Received opcode=%d", data->op_code); // 1-text; 2-binary
//         ESP_LOGW(TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
//         ESP_LOGW(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);

        if (data->data_len && app->ext->on_websock_data) (app->ext->on_websock_data)(app, data);
        break;
    case WEBSOCKET_EVENT_ERROR:
        if (app->ext->on_websock_error) (app->ext->on_websock_error)(app, data);
        break;
    default:
        ESP_LOGE(TAG, "Websocket event unhandled: %d", event_id);
        app->exited = true;
        break;
    }
}

void wscli_app_websocket_start(wifi_app_t* app) {

    shutdown_signal_timer = xTimerCreate("Websocket shutdown timer", app->ext->no_data_timeout_sec * 1000 / portTICK_PERIOD_MS,
                                         pdFALSE, NULL, wscli_app_shutdown_signaler);
    shutdown_sema = xSemaphoreCreateBinary();

    const size_t buff_host_size = 200;
    char host_buff[buff_host_size];

    char wsuri_buff[buff_host_size+50];

    ESP_ERROR_CHECK( wscli_app_host_secret_cid_load(app->nvs_handle, host_buff, buff_host_size, app->ext->secret, WSCLI_APP_SECRET_SIZE, app->ext->cid, WSCLI_APP_CID_SIZE) );
    snprintf(wsuri_buff, buff_host_size+50, app->ext->wsuri_fmt, host_buff);
    app->ext->websocket_cfg.uri = wsuri_buff;
    app->ext->websocket_cfg.pingpong_timeout_sec = 3;
    
    ESP_LOGI(TAG, "Connecting to %s...", app->ext->websocket_cfg.uri);

    app->ext->client = esp_websocket_client_init(&app->ext->websocket_cfg);
    if (!app->ext->client) app->ext->on_websock_error(app, NULL);
    ESP_ERROR_CHECK_WITHOUT_ABORT( esp_websocket_register_events(app->ext->client, WEBSOCKET_EVENT_ANY, wscli_app_websocket_event_handler, (void *)app) );

    ESP_ERROR_CHECK_WITHOUT_ABORT( esp_websocket_client_start(app->ext->client) );
    xTimerStart(shutdown_signal_timer, portMAX_DELAY);
}

void wscli_app_websocket_stop(wifi_app_t* app) {
    xSemaphoreTake(shutdown_sema, portMAX_DELAY);
    ESP_ERROR_CHECK_WITHOUT_ABORT( esp_websocket_client_stop(app->ext->client) );
    ESP_LOGI(TAG, "Websocket Stopped");
    ESP_ERROR_CHECK_WITHOUT_ABORT( esp_websocket_client_destroy(app->ext->client) );
}



void wscli_app_loop(void* arg) {
    wifi_app_t* app = arg;
    if (wifi_disconnected_prev != wifi_disconnected) {
        // ESP_LOGI(TAG, "WIFI STATUS CHANGED TO %s\n", wifi_disconnected ? "DESCONNECTED" : "CONNECTED");
        if (wifi_disconnected) wscli_app_websocket_stop(app); 
        else wscli_app_websocket_start(app);
        wifi_disconnected_prev = wifi_disconnected;
        if (wifi_disconnected && app->ext->on_wifi_disconnected) (app->ext->on_wifi_disconnected)(app);
        else if (!wifi_disconnected && app->ext->on_wifi_connected) (app->ext->on_wifi_connected)(app);
    } else {
        // ESP_LOGI(TAG, "WIFI STATUS IS %s\n", wifi_disconnected ? "DESCONNECTED" : "CONNECTED");
        wifi_disconnected_prev = wifi_disconnected;
        if(app->ext->on_websock_loop) (app->ext->on_websock_loop)(app);
    }
}


// ------------------------------------------------------
// WSCLI EVENT HANDLERS
// ------------------------------------------------------

void wscli_app_on_wifi_connected(void* arg) {
    // wifi_app_t* app = arg; // using argument as an app
    // ESP_LOGI(TAG, "----------- [WIFI CONNECTED] -------------");
}

void wscli_app_on_wifi_disconnected(void* arg) {
    // wifi_app_t* app = arg; // using argument as an app
    // ESP_LOGI(TAG, "----------- [WIFI DISCONNECTED] -------------");
}

void wscli_app_on_websock_connected(void* arg) {
    wifi_app_t* app = arg; // using argument as an app
    ESP_LOGI(TAG, "----------- [WEBSOCKET CONNECTED] -------------");
    ESP_ERROR_CHECK_WITHOUT_ABORT( websock_sendf(app->ext->client, "HELLO %s %s", app->ext->secret, app->ext->cid) );
    
}

void wscli_app_on_websock_disconnected(void* arg) {
    wifi_app_t* app = arg; // using argument as an app
    ESP_LOGI(TAG, "----------- [WEBSOCKET DISCONNECTED] -------------");
    app->ext->sys->streaming = false;
}

void wscli_app_on_websock_data(void* arg, esp_websocket_event_data_t* data) {
    wifi_app_t* app = arg; // using argument as an app
    ESP_LOGI(TAG, "----------- [WEBSOCKET DATA] -------------");

    ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
    ESP_LOGI(TAG, "Received opcode=%d", data->op_code); // 1-text; 2-binary
    ESP_LOGW(TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
    ESP_LOGW(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);

    size_t response_size = 1000;
    char response_buff[response_size];
    int outlen = 0; response_buff[0] = '\0';

    if (!strcmp(data->data_ptr, "?UPDATE")) {

        camsys_t* sys = app->ext->sys;

        size_t diff_sum_max = watcher.diff_sum_max;
        watcher.diff_sum_max = 0;
        
        outlen = snprintf(response_buff, response_size, 
            "{\"func\":\"update\",\"mode\":\"%s\",\"streaming\":%s,\"camera\":{\"recording\":%s},\"watcher\":{\"x\":%d,\"y\":%d,\"size\":%d,\"raster\":%d,\"threshold\":%d,\"diff_sum_max\":%d}}", 
            (sys->mode == CAMSYS_MODE_CAMERA ? "camera" : "motion"),
            (sys->streaming ? "true" : "false"),
            (sys->camera->file ? "true" : "false"),
            watcher.x, watcher.y, watcher.size, watcher.raster, watcher.threshold, diff_sum_max
        );

    } else if (!strcmp(data->data_ptr, "!STREAM STOP")) {

        app->ext->sys->streaming = false;
        outlen = snprintf(response_buff, response_size, "{\"func\":\"msg\",\"msg\":\"stream stop\"}");

    } else if (!strcmp(data->data_ptr, "!RECORD START")) {

        esp_err_t err = camera_recording_start(app->ext->sys->camera);
        if (err != ESP_OK) outlen = snprintf(response_buff, response_size, "{\"func\":\"err\",\"msg\":\"record start error: '%d'\"}", err);
        else outlen = snprintf(response_buff, response_size, "{\"func\":\"msg\",\"msg\":\"record start\"}");

    } else if (!strcmp(data->data_ptr, "!RECORD STOP")) {

        esp_err_t err = camera_recording_stop(app->ext->sys->camera);
        if (err != ESP_OK) outlen = snprintf(response_buff, response_size, "{\"func\":\"err\",\"msg\":\"record stop error: '%d'\"}", err);
        else outlen = snprintf(response_buff, response_size, "{\"func\":\"msg\",\"msg\":\"record stop\"}");

    } else if (!strcmp(data->data_ptr, "!RECORD DELETE")) {

        esp_err_t err = camera_recording_delete(app->ext->sys->camera);
        if (err != ESP_OK) outlen = snprintf(response_buff, response_size, "{\"func\":\"err\",\"msg\":\"record delete error: '%d'\"}", err);
        else outlen = snprintf(response_buff, response_size, "{\"func\":\"msg\",\"msg\":\"record deleted\"}");

    } else if (str_starts_with(data->data_ptr, "!WATCH ")) {
        char buff[100];
        strncpy_offs(buff, strlen("!WATCH "), data->data_ptr, 99);
        ESP_LOGI(TAG, "watch data: '%s'", buff);
        
        int i = 0; // TODO make a function
        int alen = 5;
        const char* tok = "/";
        char *p = strtok (buff, tok);
        char *array[alen];

        while (p != NULL && i<alen)
        {
            array[i++] = p;
            p = strtok (NULL, tok);
        }

        watcher.x = atoi(array[0]); // TODO outsource into a sep function
        watcher.y = atoi(array[1]);
        watcher.size = atoi(array[2]);
        watcher.raster = atoi(array[3]);
        watcher.threshold = atoi(array[4]);
        watcher.diff_sum_max = 0;
        camsys_motion_websock_loop_first = true;
        camsys_motion_websock_loop_alert = false;

    } else {

        outlen = snprintf(response_buff, response_size, "{\"func\":\"err\",\"msg\":\"Illegal request: '%s'\"}", (char*)data->data_ptr);
    
    }
    
    if (-1 >= outlen || -1 >= esp_websocket_client_send_text(app->ext->client, response_buff, outlen, portMAX_DELAY)) {
        ESP_LOGE(TAG, "Message error, outlen:%d", outlen);
    }

    // const char* response_fmt = "ECHO: %s\n\0";
    // const size_t response_size = data->data_len + strlen(response_fmt) + 1;
    // char response_buff[response_size];
    // int outlen = snprintf(response_buff, response_size, response_fmt, data->data_ptr);
    // if (-1 == esp_websocket_client_send_text(app->ext->client, response_buff, outlen, portMAX_DELAY)) {
    //     ESP_LOGE(TAG, "Message error");
    // }
}

void wscli_app_on_websock_error(void* arg, esp_websocket_event_data_t* data) {
    // wifi_app_t* app = arg; // using argument as an app
    ESP_LOGI(TAG, "----------- [WEBSOCKET ERROR] -------------");
}

void wscli_app_on_websock_loop(void* arg) {
    wifi_app_t* app = arg; // using argument as an app

    CAMSYS_BY_APP_MODE(
        camsys_camera_websock_loop(app);, 
        camsys_motion_websock_loop(app);
    );
    // if (app->ext->sys->streaming) {
    //     camera_fb_t* fb = esp_camera_fb_get();
    //     if (fb) {
    //         if (app->ext->sys->streaming && fb->len != esp_websocket_client_send_bin(app->ext->client, (char*)fb->buf, fb->len, portMAX_DELAY)) ESP_LOGE(TAG, "Image send failed");
    //         if (fb->format != PIXFORMAT_JPEG) free(fb->buf);
    //         esp_camera_fb_return(fb);
    //     } else ESP_LOGE(TAG, "Camera capture failed");
    // }

}


// ------------------------------------------------------

void wscli_app_main(wscli_app_t* ext) {
    wifi_app_t app;
    app.namespace = CAMSYS_NAMESPACE;
    app.exited = false; 
    app.ext = ext;
    wifi_app_main(&app);
}

// ----------------------------

void camsys_app_main(camsys_t* sys)
{
    wscli_app_t ext;

    ext.no_data_timeout_sec = 3;
    ext.wsuri_fmt = "ws://%s";
    memset(&ext.websocket_cfg, 0, sizeof(ext.websocket_cfg));
    ext.client = NULL;

    ext.settings_func = wscli_app_settings;
    ext.loop_func = wscli_app_loop;

    ext.on_wifi_connected = wscli_app_on_wifi_connected;
    ext.on_wifi_disconnected = wscli_app_on_wifi_disconnected;
    ext.on_websock_connected = wscli_app_on_websock_connected;
    ext.on_websock_disconnected = wscli_app_on_websock_disconnected;
    ext.on_websock_data = wscli_app_on_websock_data;
    ext.on_websock_error = wscli_app_on_websock_error;
    ext.on_websock_loop = wscli_app_on_websock_loop;

    ext.sys = sys;
    wscli_app_main(&ext);

}

void app_main() {
    //esp_log_level_set("*", ESP_LOG_NONE);

    camsys_t sys;
    // sys.mode = CAMSYS_MODE_MOTION;
    // sys.mode = CAMSYS_MODE_CAMERA;
    sys.streaming = false;

    camsys_camera_t camera;
    camera_recording_init(&camera);

    camsys_motion_t motion;

    sys.camera = &camera;
    sys.motion = &motion;
    camsys_app_main(&sys);

    fflush(stdout);
    delay(1000);
    esp_restart();
}