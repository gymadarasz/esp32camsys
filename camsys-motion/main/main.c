#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"

/************************ COMMON **************************/

char* get_settings_nvs() {
    
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
    printf("Opening Non-Volatile Storage (NVS) handle... \n");
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
                    printf("alloc error\n");
                    // Close
                    nvs_close(nvs_handle);
                    return NULL;
                }
                err = nvs_get_str(nvs_handle, "wifi", old_data, &size);
                if (err != ESP_OK) {
                    printf("retrieve error\n");
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

int get_str_pieces(const char* str, char delim) {
    int i=0, pieces = 1;
    while(str[i]) {
        if(str[i] == delim) pieces++;
        i++;
    }
    return pieces;
}

int get_str_piece_at(char* buff, const char* str, char delim, int pos) {
    int i=0, j=0, p = 0;
    buff[j] = 0;
    while(str[i]) {
        if (p == pos) {
            if(str[i] == delim) break;
            buff[j] = str[i];
            j++;
        }
        if(str[i] == delim) p++;
        i++;
    }
    buff[j] = 0;
    return j;
}

bool get_settings_wifi_password(char* pwdbuf, char* settings_nvs, const char* ssid) {    
    char strbuf[1000];
    get_str_piece_at(strbuf, settings_nvs, '\n', 0);
    
    int wifinum = get_str_pieces(strbuf, ';');
    printf("%d WIFI Settings found.\n", wifinum);
    
    for (int i=0; i<wifinum; i++) {
        char cred[100];
        get_str_piece_at(cred, strbuf, ';', i);
        get_str_piece_at(pwdbuf, cred, ':', 0);
        printf("Check '%s' (%d)\n", pwdbuf, i);
        if (!strcmp(pwdbuf, ssid)) {
            get_str_piece_at(pwdbuf, cred, ':', 1);
            printf("Password for Wifi SSID (%s) is found.\n", ssid);
            return true;
        }
    }
    
    return false;
}

/************************ WIFI **************************/
#define WIFI_RECONNECT_ATTEMPTS 20

static bool wifi_scan_done = false;
static bool wifi_connected = false;
static int wifi_reconnect_attempts = WIFI_RECONNECT_ATTEMPTS;
static bool wifi_got_ip = false;
//static EventGroupHandle_t s_connect_event_group;
static ip4_addr_t wifi_ip_addr;
//static const char *s_connection_name;
static char* settings_nvs;

static void wifi_scan_init();
static bool wifi_scan_start(int ap_max, int max_try);



static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    printf("WIFI EVENT: %d\n", event_id);
    
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        ESP_LOGI("camsys", "wifi_scan_done.");
        ESP_ERROR_CHECK(esp_wifi_scan_stop());
        wifi_scan_done = true;
    }
    
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI("camsys", "IP given!");
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        memcpy(&wifi_ip_addr, &event->ip_info.ip, sizeof(wifi_ip_addr));
        //xEventGroupSetBits(s_connect_event_group, BIT(0));
        ESP_ERROR_CHECK(esp_wifi_scan_stop());
        wifi_connected = true;
        wifi_got_ip = true;
    }
    
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI("camsys", "wifi_connected.");
        ESP_ERROR_CHECK(esp_wifi_scan_stop());
        wifi_reconnect_attempts = WIFI_RECONNECT_ATTEMPTS;
        wifi_connected = true;
    }
    
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        wifi_got_ip = false;
        
        ESP_LOGI("camsys", "Wi-Fi disconnected, trying to reconnect...");
        printf("Reconnect attempts left: %d\n", wifi_reconnect_attempts);
        wifi_reconnect_attempts--;
        if (wifi_reconnect_attempts < 0) {
            printf("ERROR: Connection lost for so long, restarting....\n");
            esp_restart();
        }
        ESP_ERROR_CHECK(esp_wifi_connect());
        
        //ESP_ERROR_CHECK(esp_wifi_scan_stop());
        //ESP_ERROR_CHECK(esp_wifi_stop());
        
        //wifi_scan_init();
        //while (!wifi_scan_start(10, 0));
        //printf("RE-Connected to WIFI.\n");
    }
    
}

bool wifi_connect(const char* ssid, const char* pwd) {
    printf("WIFI Connecting...\n");
    //if (s_connect_event_group != NULL) {
    //    printf("ERROR: s_connect_event_group already set\n");
    //    return false;
    //}
    //printf("xEventGroupCreate...\n");
    //s_connect_event_group = xEventGroupCreate();

    
    printf("esp_wifi_init... (in wifi_connect)\n");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    //printf("esp_event_handler_register [wifi_event_handler]...\n");
    //ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    //printf("esp_event_handler_register [on_wifi_disconnect]...\n");
    //ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect, NULL));
    
    //printf("esp_event_handler_register [on_got_ip]...\n");
    //ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL));
    
    printf("esp_wifi_set_storage...\n");
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    wifi_config_t wifi_config =  {
         .sta = {
                 .ssid = "",
                 .password = "",
                 .bssid_set = 0
             }
     };

    strncpy((char*)wifi_config.sta.ssid, ssid, 32);
    strncpy((char*)wifi_config.sta.password, pwd, 64);
    
    printf("Connecting to %s...\n", wifi_config.sta.ssid);
    printf("esp_wifi_set_config...\n");
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    printf("esp_wifi_connect...\n");
    ESP_ERROR_CHECK(esp_wifi_connect());
    //s_connection_name = ssid;
    
    //printf("xEventGroupWaitBits...\n");
    //xEventGroupWaitBits(s_connect_event_group, BIT(0), true, false, portMAX_DELAY);
    printf("Waiting for WIFI connection established...\n");
    while(!wifi_connected);
    ESP_LOGI("camsys", "Connected to %s", ssid);
    ESP_LOGI("camsys", "IPv4 address: " IPSTR, IP2STR(&wifi_ip_addr));
    return true;
}






/* Initialise a wifi_ap_record_t, get it populated and display scanned data */
bool wifi_connect_attempts(char* settings_nvs, uint16_t ap_max)
{
    if (ap_max < 1) ap_max = 1;
    wifi_ap_record_t ap_info[ap_max];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_max, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI("camsys", "Total APs scanned = %u", ap_count);
    for (int i = 0; (i < ap_max) && (i < ap_count); i++) {
        ESP_LOGI("camsys", "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI("camsys", "RSSI \t\t%d", ap_info[i].rssi);
        ESP_LOGI("camsys", "Channel \t\t%d\n", ap_info[i].primary);
        printf("Attempt to connect to '%s'...\n", ap_info[i].ssid);
        
        char pwd[40];
        if (get_settings_wifi_password(pwd, settings_nvs, (char*)ap_info[i].ssid)) {
            printf("Password found.\n");
        } else {
            printf("Password not found.\n");
            pwd[0] = 0;
        }
        // TODO: 
        return wifi_connect((char*)ap_info[i].ssid, pwd);
    }
    return false;
}

static void wifi_scan_init() {
    
    printf("esp_wifi_init... (int wifi_scan_init)\n");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    //printf("esp_event_handler_register [wifi_event_handler]...\n");
    //ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    //printf("esp_event_handler_register [on_wifi_disconnect]...\n");
    //ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect, NULL));
    
    //printf("esp_event_handler_register [on_got_ip]...\n");
    //ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL));
    
}

/* Initialize Wi-Fi as sta and start scan */
// ap_max: scan up to ap_max wifi access point
// max_try: try to connect max_try times before gives up
// return: true if connected
static bool wifi_scan_start(int ap_max, int max_try)
{
    if (wifi_connected || wifi_got_ip) {
        printf("WARN: Trying to scan WIFI when already connected.");
        return true;
    }
    bool connected = false;
    
    while(!connected) {
        wifi_connected = false;
        wifi_got_ip = false;
        wifi_scan_done = false;        
        //printf("esp_event_handler_register [wifi_event_handler]...\n");
        //ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

        printf("Scanning WIFI APs...\n");
        ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
        printf("Waiting for scan results...\n");
        while (!wifi_scan_done) {
            vTaskDelay(100 / portTICK_RATE_MS);
            max_try--;
            if (!max_try) break;
            if (max_try < 0) max_try = 0;
        }        
        ESP_ERROR_CHECK(esp_wifi_scan_stop());
        printf("WIFI APs are scanned.\n");
        connected = wifi_connect_attempts(settings_nvs, ap_max);
    }
    
    return connected;
}


#define WIFI_SETUP_UART_TXD         (GPIO_NUM_1)
#define WIFI_SETUP_UART_RXD         (GPIO_NUM_3)
#define WIFI_SETUP_UART_RTS         (UART_PIN_NO_CHANGE)
#define WIFI_SETUP_UART_CTS         (UART_PIN_NO_CHANGE)
#define WIFI_SETUP_UART_BUFF_SIZE   (4096)

bool set_settings_nvs(const char* data) {
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
    printf("Opening Non-Volatile Storage (NVS) handle... \n");
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
                    printf("alloc error\n");
                    // Close
                    nvs_close(nvs_handle);
                    return false;
                }
                err = nvs_get_str(nvs_handle, "wifi", old_data, &size);
                if (err != ESP_OK) {
                    printf("retrieve error\n");
                    // Close
                    free(old_data);
                    nvs_close(nvs_handle);
                    return false;
                }
                if (!strcmp(data, old_data)) {
                    printf("old data was the same\n");
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
        printf("Updating wifi data in NVS ... \n");
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
        printf("Committing updates in NVS ... \n");
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
    //char* settings_nvs = get_settings_nvs();
    //printf("Current settings: %s\n", settings_nvs ? settings_nvs : "(NULL)");
    //free(settings_nvs);
    
    printf("Ready to retrieve setup data...\n");
    do {
        len = uart_read_bytes(UART_NUM_0, data, WIFI_SETUP_UART_BUFF_SIZE, 20 / portTICK_RATE_MS);
        data[len] = 0;
    } while(len <= 0);
    
    if (set_settings_nvs((char*)data)) {
        printf("ECHO:%s\n", data);
    } else {
        printf("ERROR:Error on settings update (may it's not changed)\n");
    }
    free(data);
    while(1);
}

void wifi_auto_start() {
    settings_nvs = get_settings_nvs();
    printf("NVS Settings are retrieved.\n");
    
    printf("tcpip_adapter_init...\n");
    tcpip_adapter_init();

    printf("esp_event_loop_create_default...\n");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    printf("esp_event_handler_register [wifi_event_handler]...\n");
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    
    wifi_scan_init();
    while (!wifi_scan_start(10, 0));
    printf("Connected to WIFI.\n");
    
    free(settings_nvs);
}

/************************ MOTION **************************/

void motion_sensor() {
    printf("MOTION DETECTOR MODE\n");
    wifi_auto_start();
    while(1); // TODO: main loop here
}

/************************ CAMERA **************************/

void camera_recorder() {
    printf("CAMERA RECORDER MODE\n");
    wifi_auto_start();
    while(1); // TODO: main loop here
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



/************************ TESTS **************************/

void test_error(int line) {
    printf("ERROR: Test failed at %d. Restart...\n", line);
    esp_restart();
}

int tests()
{
    printf("\n ----[TESTS]---- \n");
    
    char* str = "asd/qwe/zxc";    
    int pieces = get_str_pieces(str, '/');
    if (pieces != 3) test_error(__LINE__); else printf(".");
    
    char piece[100];
    int len = get_str_piece_at(piece, str, '/', 2);
    if (strcmp(piece, "zxc")) test_error(__LINE__); else printf(".");
    if (len != 3) test_error(__LINE__); else printf(".");
    
    char pwdbuf[100];
    if (!get_settings_wifi_password(pwdbuf, "ssid123:test123;ssid123_EXT:test1234\n192.168.0.104", "ssid123_EXT")) test_error(__LINE__); else printf(".");
    if (strcmp(pwdbuf, "test1234")) test_error(__LINE__); else printf(".");
    
    printf("\n ----[TESTS PASSED]---- \n");
    return 0;
}

void app_main()
{
    tests();
    main_task();
    //xTaskCreate(main_task, "main_task", 1024, NULL, 10, NULL);
}
