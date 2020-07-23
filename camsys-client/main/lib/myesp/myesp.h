#ifndef MYESP_H
#define MYESP_H

#include <stdarg.h> 
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"


#define MYESP_NAMESPACE "myesp"

// ---------------------------------------------------------------
// DELAY
// ---------------------------------------------------------------

void delay(long ms);

// ---------------------------------------------------------------
// ERROR
// ---------------------------------------------------------------

#define ERROR_BUFF_SIZE 100

#define ERROR(msg) errorlnf(__LINE__, msg)
#define ERRORF(fmt, ...) errorlnf(__LINE__, fmt, __VA_ARGS__)

void errorlnf(int line, const char* fmt, ...);

// ---------------------------------------------------------------
// SERIAL READ
// ---------------------------------------------------------------

#define SERIAL_READLN_BUFF_SIZE 255

esp_err_t serial_install(uart_port_t port, esp_line_endings_t in_line_ending, esp_line_endings_t out_line_ending);
esp_err_t serial_uninstall(uart_port_t port);
char* serial_readln();

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

esp_err_t sdcard_init(sdcard_mount_func_t mounter, bool auto_format, int max_files, size_t alloc_unit_size);
esp_err_t sdcard_close(sdcard_unmount_func_t unmounter);

// ---------------------------------------------------------------
// STRING EXTRAS
// ---------------------------------------------------------------

char* strncpy_offs(char* dest, size_t offs, const char* src, size_t max);  // TODO: it's a string helper

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

void wifi_creds_clear(wifi_creds_t creds);
char* wifi_creds_get_pswd(wifi_creds_t creds, const char* ssid);

#endif // MYESP_H