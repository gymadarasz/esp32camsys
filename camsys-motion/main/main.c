#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"

#define WIFI_SETUP_UART_TXD         (GPIO_NUM_1)
#define WIFI_SETUP_UART_RXD         (GPIO_NUM_3)
#define WIFI_SETUP_UART_RTS         (UART_PIN_NO_CHANGE)
#define WIFI_SETUP_UART_CTS         (UART_PIN_NO_CHANGE)
#define WIFI_SETUP_UART_BUFF_SIZE   (4096)

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
    
    printf("READY TO WIFI SETUP\n");
    do {
        len = uart_read_bytes(UART_NUM_0, data, WIFI_SETUP_UART_BUFF_SIZE, 20 / portTICK_RATE_MS);
        data[len] = 0;
    } while(len <= 0);
    
    // TODO: save wifi to eprom
        
    printf("ECHO:%s\n", data);
    free(data);
    while(1);
}

void motion_sensor() {
    printf("MOTION DETECTOR MODE\n");
    while(1);
}

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
    xTaskCreate(main_task, "main_task", 1024, NULL, 10, NULL);
}
