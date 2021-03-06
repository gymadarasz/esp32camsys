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
#include "esp_websocket_client.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"

/************************ COMMON **************************/

int get_str_pieces(const char* str, char delim) {
    int i=0, pieces = 1;
    while(str[i]) {
        if(str[i] == delim) pieces++;
        i++;
    }
    return pieces;
}

int get_str_piece_at(char* buff, size_t size, const char* str, char delim, int pos) {
    int i=0, j=0, p = 0;
    buff[j] = 0;
    while(str && str[i]) {
        if (p == pos) {     
            if(str[i] == delim) break;
            buff[j] = str[i];
            if (size <= j) break;
            j++;
        }
        if(str[i] == delim) p++;
        i++;
    }
    buff[j] = 0;
    return j;
}

/************************ ESP SPECIFIC COMMON **************************/

char* get_mac_addr(char* buff, size_t size) {
    uint8_t mac[20];
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac));
    snprintf(buff, size, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); // TODO: hope it's unigue enough
    return buff;
}


/************************ NVS **************************/


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


bool get_settings_wifi_password(char* pwdbuf, size_t pwdbuff_size, const char* settings_nvs, const char* ssid) {    
    size_t size = 1000;
    char strbuf[size];
    get_str_piece_at(strbuf, size, settings_nvs, '\n', 0);
    
    int wifinum = get_str_pieces(strbuf, ';');
    printf("%d WIFI Settings found.\n", wifinum);
    
    for (int i=0; i<wifinum; i++) {
        size_t cred_size = 100;
        char cred[cred_size];
        get_str_piece_at(cred, cred_size, strbuf, ';', i);
        get_str_piece_at(pwdbuf, pwdbuff_size, cred, ':', 0);
        printf("Check '%s' (%d)\n", pwdbuf, i);
        if (!strcmp(pwdbuf, ssid)) {
            get_str_piece_at(pwdbuf, pwdbuff_size, cred, ':', 1);
            printf("Password for Wifi SSID (%s) is found.\n", ssid);
            return true;
        }
    }
    
    return false;
}

char* get_settings_server_uri(char* buff, size_t size, const char* settings_nvs) {
    get_str_piece_at(buff, size, settings_nvs, '\n', 1);
    return buff;
}



/************************ WIFI SETUP AND AUTO CONNECT **************************/

#define WIFI_SETUP_UART_TXD         (GPIO_NUM_1)
#define WIFI_SETUP_UART_RXD         (GPIO_NUM_3)
#define WIFI_SETUP_UART_RTS         (UART_PIN_NO_CHANGE)
#define WIFI_SETUP_UART_CTS         (UART_PIN_NO_CHANGE)
#define WIFI_SETUP_UART_BUFF_SIZE   (4096)


#define WIFI_RECONNECT_ATTEMPTS 30
#define WIFI_SCAN_MAX_AP 10

static bool wifi_scan_done = false;
static bool wifi_connected = false;
static int wifi_reconnect_attempts = WIFI_RECONNECT_ATTEMPTS;
static bool wifi_got_ip = false;
//static EventGroupHandle_t s_connect_event_group;
static ip4_addr_t wifi_ip_addr;
//static const char *s_connection_name;

static void wifi_scan_init();
static bool wifi_scan_start(const char* settings_nvs, int ap_max, int max_try);


static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    printf("WIFI_EVENT ID: %d\n", event_id);
    
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        ESP_LOGI("camsys", "wifi_scan_done.");
        ESP_ERROR_CHECK(esp_wifi_scan_stop());
        wifi_scan_done = true;
    }
    
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI("camsys", "wifi_connected.");
        ESP_ERROR_CHECK(esp_wifi_scan_stop());
        wifi_reconnect_attempts = WIFI_RECONNECT_ATTEMPTS;
        wifi_connected = true;
    }
    
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {  
        wifi_connected = false; 
        
        ESP_LOGI("camsys", "Wi-Fi disconnected, trying to reconnect...");
        printf("Reconnect attempts left: %d\n", wifi_reconnect_attempts);
        wifi_reconnect_attempts--;
        if (wifi_reconnect_attempts < 0) {
            
            ESP_ERROR_CHECK(esp_wifi_scan_stop());
            ESP_ERROR_CHECK(esp_wifi_stop());
                
            wifi_scan_init();
            char* settings_nvs = get_settings_nvs();
            while (!wifi_scan_start(settings_nvs, WIFI_SCAN_MAX_AP, 0));
            free(settings_nvs);
            
            if (!wifi_connected) {
                printf("ERROR: Connection lost for so long, restarting....\n");
                esp_restart();
            } 
            printf("RE-Connected to WIFI.\n");  
            
        } else ESP_ERROR_CHECK(esp_wifi_connect());      
    }
    
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    printf("IP_EVENT ID: %d\n", event_id);
    
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI("camsys", "IP given!");
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        memcpy(&wifi_ip_addr, &event->ip_info.ip, sizeof(wifi_ip_addr));
        //xEventGroupSetBits(s_connect_event_group, BIT(0));
        ESP_ERROR_CHECK(esp_wifi_scan_stop());
        wifi_got_ip = true;
    }
    
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
        ESP_LOGI("camsys", "IP lost!");
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        memcpy(&wifi_ip_addr, &event->ip_info.ip, sizeof(wifi_ip_addr));
        //xEventGroupSetBits(s_connect_event_group, BIT(0));
        ESP_ERROR_CHECK(esp_wifi_scan_stop());
        wifi_got_ip = false;
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

    
    //printf("esp_wifi_init... (in wifi_connect)\n");
    //wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    //ESP_ERROR_CHECK(esp_wifi_init(&cfg));


    //printf("esp_event_handler_register [wifi_event_handler]...\n");
    //ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    //printf("esp_event_handler_register [on_wifi_disconnect]...\n");
    //ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect, NULL));
    
    //printf("esp_event_handler_register [on_got_ip]...\n");
    //ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL));
    
    //printf("esp_wifi_set_storage...\n");
    //ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    //ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
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
    while(!wifi_got_ip);
    ESP_LOGI("camsys", "Connected to %s", ssid);
    ESP_LOGI("camsys", "IPv4 address: " IPSTR, IP2STR(&wifi_ip_addr));
    return true;
}



/* Initialise a wifi_ap_record_t, get it populated and display scanned data */
bool wifi_connect_attempts(const char* settings_nvs, uint16_t ap_max)
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
        
        size_t pwd_size = 40;
        char pwd[pwd_size];
        if (get_settings_wifi_password(pwd, pwd_size, settings_nvs, (char*)ap_info[i].ssid)) {
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
static bool wifi_scan_start(const char* settings_nvs, int ap_max, int max_try)
{
    if (wifi_connected) {
        printf("WARN: Trying to scan WIFI when already connected.\n");
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

void wifi_auto_start(const char* settings_nvs) {
    
    printf("tcpip_adapter_init...\n");
    tcpip_adapter_init();

    printf("esp_event_loop_create_default...\n");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    printf("esp_event_handler_register [wifi_event_handler]...\n");
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    printf("esp_event_handler_register [ip_event_handler]...\n");
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL));
    
    wifi_scan_init();
    while (!wifi_scan_start(settings_nvs, WIFI_SCAN_MAX_AP, 0));
    printf("Connected to WIFI.\n");
    
}

/************************ WEBSOCKET CLIENT **************************/

#define NO_DATA_TIMEOUT_SEC 30

static SemaphoreHandle_t shutdown_sema;
static TimerHandle_t shutdown_signal_timer;


void wifi_websocket_client_app_connected(esp_websocket_client_handle_t client);
void wifi_websocket_client_app_disconnected(esp_websocket_client_handle_t client);
void wifi_websocket_client_app_error(esp_websocket_client_handle_t client);
void wifi_websocket_client_app_data(esp_websocket_client_handle_t client, esp_websocket_event_data_t *data);

static void shutdown_signaler(TimerHandle_t xTimer)
{
    ESP_LOGI("camsys", "No data received for %d seconds, signaling shutdown", NO_DATA_TIMEOUT_SEC);
    xSemaphoreGive(shutdown_sema);
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    printf("WEBSOCKT EVENT ID: %d\n", event_id);
    
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI("camsys", "WEBSOCKET_EVENT_CONNECTED");
        wifi_websocket_client_app_connected(handler_args);
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI("camsys", "WEBSOCKET_EVENT_DISCONNECTED");
        wifi_websocket_client_app_disconnected(handler_args);
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI("camsys", "WEBSOCKET_EVENT_DATA");
        ESP_LOGI("camsys", "Received opcode=%d", data->op_code);
        ESP_LOGW("camsys", "Received=%.*s", data->data_len, (char *)data->data_ptr);
        ESP_LOGW("camsys", "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);
        if (data->data_len) wifi_websocket_client_app_data(handler_args, data);
        xTimerReset(shutdown_signal_timer, portMAX_DELAY);
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI("camsys", "WEBSOCKET_EVENT_ERROR");
        wifi_websocket_client_app_error(handler_args);
        break;
    }
}

esp_websocket_client_handle_t websocket_start(const char* settings_nvs)
{
    esp_websocket_client_config_t websocket_cfg = {};

    shutdown_signal_timer = xTimerCreate("Websocket shutdown timer", NO_DATA_TIMEOUT_SEC * 1000 / portTICK_PERIOD_MS, pdFALSE, NULL, shutdown_signaler);
    shutdown_sema = xSemaphoreCreateBinary();

    size_t strsize = 1000;
    char strbuf[strsize];
    websocket_cfg.uri = get_settings_server_uri(strbuf, strsize, settings_nvs);

    ESP_LOGI("camsys", "Connecting to websocket: %s...", websocket_cfg.uri);

    esp_websocket_client_handle_t client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);

    esp_websocket_client_start(client);
    xTimerStart(shutdown_signal_timer, portMAX_DELAY);
    while(!esp_websocket_client_is_connected(client)) vTaskDelay(1000 / portTICK_RATE_MS);
    //char data[32];
    //int i = 0;
    //while (i < 100) {
    //    if (esp_websocket_client_is_connected(client)) {
    //        int len = sprintf(data, "hello %04d\n", i++);
    //        ESP_LOGI("camsys", "Sending %s", data);
    //        esp_websocket_client_send_text(client, data, len, portMAX_DELAY);
    //    }
    //    vTaskDelay(1000 / portTICK_RATE_MS);
    //}

    return client;
}

/************************ BOILERPLATE **************************/


typedef void (*wifi_websocket_client_app_setup_handler_t)(esp_websocket_client_handle_t client, char* settings_nvs);
typedef void (*wifi_websocket_client_app_event_handler_t)(esp_websocket_client_handle_t client);
typedef void (*wifi_websocket_client_app_data_handler_t)(esp_websocket_client_handle_t client, esp_websocket_event_data_t *data);

static bool wifi_websocket_client_app_exited = false;
static wifi_websocket_client_app_event_handler_t wifi_websocket_client_app_connected_cb;
static wifi_websocket_client_app_event_handler_t wifi_websocket_client_app_disconnected_cb;
static wifi_websocket_client_app_event_handler_t wifi_websocket_client_app_error_cb;
static wifi_websocket_client_app_data_handler_t wifi_websocket_client_app_data_cb;


void wifi_websocket_client_app_connected(esp_websocket_client_handle_t client) {
    printf("Websocket connected.\n");
    if (wifi_websocket_client_app_connected_cb) wifi_websocket_client_app_connected_cb(client);
}
void wifi_websocket_client_app_disconnected(esp_websocket_client_handle_t client) {
    printf("Websocket disconnected.\n");
    if (wifi_websocket_client_app_disconnected_cb) wifi_websocket_client_app_disconnected_cb(client);
}
void wifi_websocket_client_app_error(esp_websocket_client_handle_t client) {
    printf("Websocket error.\n");
    if (wifi_websocket_client_app_error_cb) wifi_websocket_client_app_error_cb(client);
}

void wifi_websocket_client_app_data(esp_websocket_client_handle_t client, esp_websocket_event_data_t *data) {
    printf("Websocket data received: '%s'\n", (char *)data->data_ptr);
    if (wifi_websocket_client_app_data_cb) wifi_websocket_client_app_data_cb(client, data);
}

int wifi_websocket_client_app_send(esp_websocket_client_handle_t client, const char* data, size_t len) {
    printf("Websocket send (binary data) in %d bytes...\n", len);
    return esp_websocket_client_send(client, data, len, portMAX_DELAY);
}

int wifi_websocket_client_app_send_text(esp_websocket_client_handle_t client, const char* text) {
    printf("Websocket send (text): '%s'...\n", text);
    return esp_websocket_client_send_text(client, text, strlen(text)+1, portMAX_DELAY);
}

int wifi_websocket_client_app_printf(esp_websocket_client_handle_t client, const char* fmt, ...) {
    char buffer[1000];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    
    printf("BUFFER:'%s'\n", buffer);
    
    int ret = wifi_websocket_client_app_send_text(client, buffer);
    
    printf("Buffer sent\n");
    
    va_end(args);
    return ret;
}

void wifi_websocket_client_app_exit() {
    printf("wifi_websocket_client_app_exit...\n");
    wifi_websocket_client_app_exited = true;
}

void wifi_websocket_client_app_stop(esp_websocket_client_handle_t client) {
    xSemaphoreTake(shutdown_sema, portMAX_DELAY);
    esp_websocket_client_stop(client);
    ESP_LOGI("camsys", "Websocket Stopped");
    esp_websocket_client_destroy(client);
}

void wifi_websocket_client_app_start(
    wifi_websocket_client_app_setup_handler_t setup_cb, 
    wifi_websocket_client_app_event_handler_t loop_cb, 
    wifi_websocket_client_app_data_handler_t data_cb,
    wifi_websocket_client_app_event_handler_t connected_cb, 
    wifi_websocket_client_app_event_handler_t disconnected_cb, 
    wifi_websocket_client_app_event_handler_t error_cb
) 
{
    char* settings_nvs = get_settings_nvs();
    printf("NVS Settings are retrieved.\n");
    printf("wifi_auto_start...\n");
    wifi_auto_start(settings_nvs);
    
    printf("websocket_start...\n");
    wifi_websocket_client_app_connected_cb = connected_cb;
    wifi_websocket_client_app_disconnected_cb = disconnected_cb;
    wifi_websocket_client_app_error_cb = error_cb;
    wifi_websocket_client_app_data_cb = data_cb;
    esp_websocket_client_handle_t client = websocket_start(settings_nvs);

    if (setup_cb) setup_cb(client, settings_nvs);
    printf("websocket event loop start...\n");
    while(!wifi_websocket_client_app_exited) {
        //printf("loop_cb...\n");
        if (loop_cb) loop_cb(client);
    }
    printf("wifi_websocket_client_app_stop...\n");
    wifi_websocket_client_app_stop(client);
    free(settings_nvs);
}

/************************ MOTION **************************/

void motion_sensor_setup(esp_websocket_client_handle_t client, char* settings_nvs) {
    printf("MOTION DETECTOR SETUP...\n");
}

void motion_sensor_loop(esp_websocket_client_handle_t client) {
    //printf("MOTION DETECTOR LOOP...\n");
}

void motion_sensor_data(esp_websocket_client_handle_t client, esp_websocket_event_data_t *data) {
    printf("MOTION DETECTOR DATA RECEIVED: '%s'\n", data->data_ptr);
}

void motion_sensor_connected(esp_websocket_client_handle_t client) {
    printf("MOTION DETECTOR CONNECTED.\n");
    char macbuf[40];
    wifi_websocket_client_app_printf(client, "HELLO MOTION %s", get_mac_addr(macbuf, sizeof(macbuf)));
}

void motion_sensor_disconnected(esp_websocket_client_handle_t client) {
    printf("MOTION DETECTOR DISCONNECTED.\n");
}

void motion_sensor_error(esp_websocket_client_handle_t client) {
    printf("MOTION DETECTOR ERROR.\n");
}

/************************ CAMERA **************************/

void camera_recorder_setup(esp_websocket_client_handle_t client, char* settings_nvs) {
    printf("CAMERA RECORDER SETUP...\n");
    char macbuf[40];
    wifi_websocket_client_app_printf(client, "HELLO CAMERA %s", get_mac_addr(macbuf, sizeof(macbuf)));
}

void camera_recorder_loop(esp_websocket_client_handle_t client) {
    printf("CAMERA RECORDER LOOP...\n");
}

void camera_recorder_data(esp_websocket_client_handle_t client, esp_websocket_event_data_t *data) {
    printf("CAMERA RECORDER DATA RECEIVED: '%s'\n", data->data_ptr);
}

void camera_recorder_connected(esp_websocket_client_handle_t client) {
    printf("CAMERA RECORDER CONNECTED.\n");
}

void camera_recorder_disconnected(esp_websocket_client_handle_t client) {
    printf("CAMERA RECORDER DISCONNECTED.\n");
}

void camera_recorder_error(esp_websocket_client_handle_t client) {
    printf("CAMERA RECORDER ERROR.\n");
}

/************************ MAIN TASK **************************/

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
    
    int motion_button_state = !gpio_get_level(GPIO_NUM_12);
    int camera_button_state = !gpio_get_level(GPIO_NUM_13);
    
    if (motion_button_state) wifi_websocket_client_app_start(motion_sensor_setup, motion_sensor_loop, motion_sensor_data, motion_sensor_connected, motion_sensor_disconnected, motion_sensor_error);
    else if (camera_button_state) wifi_websocket_client_app_start(camera_recorder_setup, camera_recorder_loop, camera_recorder_data, camera_recorder_connected, camera_recorder_disconnected, camera_recorder_error);
    else wifi_setup();
}



/************************ TESTS **************************/

void test_error(int line) {
    printf("ERROR: Test failed at %d. Restart...\n", line);
    while(1);
    esp_restart();
}

int tests()
{
    printf("\n ----[TESTS]---- \n");
    
    char* str = "asd/qwe/zxc";    
    int pieces = get_str_pieces(str, '/');
    if (pieces != 3) test_error(__LINE__); else printf(".");
    
    char piece[100];
    int len = get_str_piece_at(piece, 100, str, '/', 2);
    if (strcmp(piece, "zxc")) test_error(__LINE__); else printf(".");
    if (len != 3) test_error(__LINE__); else printf(".");
    
    char pwdbuf[100];
    if (!get_settings_wifi_password(pwdbuf, 100, "ssid123:test123;ssid123_EXT:test1234\n192.168.0.104", "ssid123_EXT")) test_error(__LINE__); else printf(".");
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
