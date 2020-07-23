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

#include "myesp.h"

// ---------------------------------------------------------------
// DELAY
// ---------------------------------------------------------------

void delay(long ms) {
    vTaskDelay(ms / portTICK_RATE_MS);
}

// ---------------------------------------------------------------
// ERROR
// ---------------------------------------------------------------

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

