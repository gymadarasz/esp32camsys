#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"

/************************ WIFI **************************/

#define WIFI_SETUP_UART_TXD         (GPIO_NUM_1)
#define WIFI_SETUP_UART_RXD         (GPIO_NUM_3)
#define WIFI_SETUP_UART_RTS         (UART_PIN_NO_CHANGE)
#define WIFI_SETUP_UART_CTS         (UART_PIN_NO_CHANGE)
#define WIFI_SETUP_UART_BUFF_SIZE   (4096)

bool set_wifi(const char* data) {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    // Open
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle_t nvs_handle;
    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");

        // Read
        size_t size;
        err = nvs_get_str(nvs_handle, "wifi", NULL, &size);
        char* old_data = NULL;
        switch (err) {
            case ESP_OK:
                old_data = malloc(size);
                if (!old_data) {
                    printf("alloc error");
                    // Close
                    nvs_close(nvs_handle);
                    return false;
                }
                err = nvs_get_str(nvs_handle, "wifi", old_data, &size);
                if (err != ESP_OK) {
                    printf("retrieve error");
                    // Close
                    free(old_data);
                    nvs_close(nvs_handle);
                    return false;
                }
                if (!strcmp(data, old_data)) {
                    printf("old data was the same");
                    // Close
                    free(old_data);
                    nvs_close(nvs_handle);
                    return true;
                }
                free(old_data);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
                break;
        }
        
        // Write
        printf("Updating wifi data in NVS ... ");
        err = nvs_set_str(nvs_handle, "wifi", data);
        if (err != ESP_OK) {
            printf("Failed!\n");
            // Close
            nvs_close(nvs_handle);
            return false;
        } else {
            printf("Done\n");
        }
        

        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        printf("Committing updates in NVS ... ");
        err = nvs_commit(nvs_handle);
        if (err != ESP_OK) {
            printf("Failed!\n");
            // Close
            nvs_close(nvs_handle);
            return false;
        } else {
            printf("Done\n");
        }

        // Close
        nvs_close(nvs_handle);
        
        return true;
    }

    return false;
}

char* get_wifi() {
    
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    
    // Open
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle_t nvs_handle;
    err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");
      
        // Read
        size_t size;
        err = nvs_get_str(nvs_handle, "wifi", NULL, &size);
        char* old_data = NULL;
        switch (err) {
            case ESP_OK:
                old_data = malloc(size);
                if (!old_data) {
                    printf("alloc error");
                    // Close
                    nvs_close(nvs_handle);
                    return NULL;
                }
                err = nvs_get_str(nvs_handle, "wifi", old_data, &size);
                if (err != ESP_OK) {
                    printf("retrieve error");
                    // Close
                    free(old_data);
                    nvs_close(nvs_handle);
                    return NULL;
                }
                return old_data;
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
                break;
        }

        // Close
        nvs_close(nvs_handle);
    }

    return NULL;
}

void wifi_setup() {

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, WIFI_SETUP_UART_TXD, WIFI_SETUP_UART_RXD, WIFI_SETUP_UART_RTS, WIFI_SETUP_UART_CTS);
    uart_driver_install(UART_NUM_0, WIFI_SETUP_UART_BUFF_SIZE * 2, 0, 0, NULL, 0);

    uint8_t *data = (uint8_t *) malloc(WIFI_SETUP_UART_BUFF_SIZE);
    int len;
    
    // TODO: SECURITY ISSUE HERE! - remove the followin part!
    char* wifi = get_wifi();
    printf("Current WIFI data: %s\n", wifi ? wifi : "(NULL)");
    free(wifi);
    
    printf("READY TO WIFI SETUP\n");
    do {
        len = uart_read_bytes(UART_NUM_0, data, WIFI_SETUP_UART_BUFF_SIZE, 20 / portTICK_RATE_MS);
        data[len] = 0;
    } while(len <= 0);
    
    if (set_wifi((char*)data)) {
        printf("ECHO:%s\n", data);
    } else {
        printf("ERROR:Error on wifi data update (may it's not changed)");
    }
    free(data);
    while(1);
}

/************************ MOTION **************************/

void motion_sensor() {
    printf("MOTION DETECTOR MODE\n");
    
    while(1);
}

/************************ CAMERA **************************/

void camera_recorder() {
    printf("CAMERA RECORDER MODE\n");
    while(1);
}

static void main_task() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<GPIO_NUM_12) | (1ULL<<GPIO_NUM_13);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    //gpio_set_direction(GPIO_NUM_12, GPIO_MODE_INPUT);
    //int mode = gpio_get_level(GPIO_NUM_12);
    
    if (!gpio_get_level(GPIO_NUM_12)) motion_sensor();
    else if (!gpio_get_level(GPIO_NUM_13)) camera_recorder();
    else wifi_setup();
}

void app_main()
{
    main_task();
    //xTaskCreate(main_task, "main_task", 1024, NULL, 10, NULL);
}
