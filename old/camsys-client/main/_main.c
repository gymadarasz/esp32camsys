#include "esp_err.h"
#include "esp_vfs_dev.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "sdkconfig.h"


/****************** COMMON *****************/

// -------- IO ---------

#define ERROR(...) error(__LINE__, __VA_ARGS__)

void println(char* msg) {
    if (msg) printf("%s\n", msg);
}

void stop(char* msg) {
    println(msg);
    esp_restart();
}

void error(int line, const char* fmt, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    printf("ERROR(%d): %s\n", line, buffer);
    esp_restart();
}


// -------- STRING ---------

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

/****************** STDIO *****************/

void stdio_init(void)
{
    // Initialize VFS & UART so we can use std::cout/cin
    if (!setvbuf(stdin, NULL, _IONBF, 0)) ERROR("Initialize VFS & UART error.");
    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install( (uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0) );
    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
}

/****************** NVS *****************/

// void nvs_init(nvs_handle_t* nvs_handle) {
//     // Initialize NVS
//     esp_err_t err = nvs_flash_init();
//     if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         // NVS partition was truncated and needs to be erased
//         // Retry nvs_flash_init
//         ESP_ERROR_CHECK(nvs_flash_erase());
//         err = nvs_flash_init();
//     }
//     ESP_ERROR_CHECK( err );

//     ESP_ERROR_CHECK( nvs_open("storage", NVS_READWRITE, nvs_handle) );
// }

// void nvs_gets(nvs_handle_t nvs_handle, const char* key, char* value_buff, size_t value_size) {
//     size_t stored_value_size = -1;
//     esp_err_t err = nvs_get_str(nvs_handle, key, NULL, &stored_value_size);
//     if (err == ESP_ERR_NVS_NOT_FOUND) {
//         value_buff[0] = '\0';
//         value_size = 0;
//         return;
//     } else if (err == ESP_OK) {
//         if (stored_value_size > value_size && !realloc(value_buff, stored_value_size)) err = !ESP_OK;
//         if (err == ESP_OK) err = nvs_get_str(nvs_handle, key, value_buff, &stored_value_size);
//     }
//     if (err != ESP_OK) nvs_close(nvs_handle);
//     ESP_ERROR_CHECK(err);
// }

// void nvs_sets(nvs_handle_t nvs_handle, const char* key, const char* valstr) {
//     esp_err_t err = nvs_set_str(nvs_handle, key, valstr);
//     if (err != ESP_OK) nvs_close(nvs_handle);
//     ESP_ERROR_CHECK(err);
// }

// void nvs_store(nvs_handle_t nvs_handle) {
//     ESP_ERROR_CHECK(nvs_commit(nvs_handle));
// }

// // --------- NVS SETTINGS ---------

// char* retrieve_settings(nvs_handle_t nvs_handle) {

//     size_t settings_size = 1000;
//     char* settings_buff = malloc(settings_size);
//     if (settings_buff) ERROR("Memory allocation error. (%ld bytes)", settings_size);
//     nvs_gets(nvs_handle, "settings", settings_buff, settings_size);

//     return settings_buff;
// }

// void get_wifi_credentials_settings(char* settings, char* buff, size_t size) {
//     get_str_piece_at(buff, size, settings, '\n', 0);
// }

// void validate_settings(char* buff) {
//     if (get_str_pieces(buff, '\n') < 2) ERROR("Incorrect settings");

//     size_t wifi_credentials_size = 1000;
//     char wifi_credentials_buff[wifi_credentials_size];
//     get_wifi_credentials_settings(buff, wifi_credentials_buff, wifi_credentials_size);
//     if (get_str_pieces(wifi_credentials_buff, ';') < 1) ERROR("Incorrect settings, missing wifi credentials");
// }

// size_t get_wifi_credentials_num_in_settings(char* settings_buff) {
    
//     validate_settings(settings_buff);

//     size_t wifi_credentials_size = 1000;
//     char wifi_credentials_buff[wifi_credentials_size];
//     get_wifi_credentials_settings(settings_buff, wifi_credentials_buff, wifi_credentials_size);

//     size_t wifi_credentials_num = get_str_pieces(wifi_credentials_buff, ';');
//     if (wifi_credentials_num < 1) ERROR("Missing wifi credentials");

//     return wifi_credentials_num;
// }

// void get_wifi_credential_from_settings_at(
//     char* settings_buff, 
//     char* ssid_buff, size_t ssid_size, 
//     char* pwd_buff, size_t pwd_size, 
//     size_t at) 
// {
//     validate_settings(settings_buff);

//     size_t wifi_credentials_size = 1000;
//     char wifi_credentials_buff[wifi_credentials_size];
//     get_wifi_credentials_settings(settings_buff, wifi_credentials_buff, wifi_credentials_size);

//     if (get_str_pieces(wifi_credentials_buff, ';') <= at) ERROR("Retrieve WiFi settings error.");

//     size_t wifi_credential_size = 100;
//     char wifi_credential_buff[wifi_credential_size];
//     get_str_piece_at(wifi_credential_buff, wifi_credential_size, wifi_credentials_buff, ';', at);

//     if (get_str_pieces(wifi_credential_buff, ':') < 2) ERROR("Incorrect Wifi credentials.");

//     get_str_piece_at(ssid_buff, ssid_size, wifi_credential_buff, ':', 0);
//     get_str_piece_at(pwd_buff, pwd_size, wifi_credential_buff, ':', 1); 

// }

// bool get_wifi_pwd(char* settings_buff, const char* ssid, char* pwd_buff, size_t pwd_size) {

//     size_t ssid_size = 32;
//     char ssid_buff[ssid_size];

//     size_t wifi_credentials_num = get_wifi_credentials_num_in_settings(settings_buff);
//     for (int i=0; i<wifi_credentials_num; i++) {
//         get_wifi_credential_from_settings_at(settings_buff, ssid_buff, ssid_size, pwd_buff, pwd_size, i);

//         if (!strcmp(ssid, ssid_buff)) {
//             return true;
//         }

//     }


//     pwd_size = 0;
//     pwd_buff[0] = '\0';

//     return false;
// }

// /****************** GPIO SETUP *****************/

// void gpio_pins_init() {
//     gpio_config_t io_conf;
//     io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
//     io_conf.mode = GPIO_MODE_INPUT;
//     io_conf.pin_bit_mask = (1ULL<<GPIO_NUM_12) | (1ULL<<GPIO_NUM_13);
//     io_conf.pull_down_en = 0;
//     io_conf.pull_up_en = 1;
//     ESP_ERROR_CHECK( gpio_config(&io_conf) );
// }

/****************** SETTINGS MODE *****************/


void main_settings(/*nvs_handle_t nvs_handle*/)
{
    //stdio_init();
    // nvs_handle_t nvs_handle;
    // nvs_init(&nvs_handle);


    println("CAMSYS SETUP\nType WiFi credentials, and host name or IP addresses.\nFormat:\nssid:password;ssid:password...\\nhost;host...\\n");
    // const size_t input_size = 1000;
    // char input_buff[input_size];
    // if (!fgets(input_buff, input_size, stdin)) ERROR("Input error");
    // validate_settings(input_buff);
    
    // char* settings_buff = retrieve_settings(nvs_handle);

    // if (!strcmp(input_buff, settings_buff)) {
    //     free(settings_buff);
    //     println("Settings are not changed.");
    // } else {
    //     free(settings_buff);
    //     nvs_sets(nvs_handle, "settings", input_buff);
    //     nvs_store(nvs_handle);
    //     println("New settings are commited.");
    // }

    // nvs_close(nvs_handle);
}

/****************** MOTION MODE *****************/





// bool wifi_connect_attempt(char* settings_buff, char* ssid) {
//     size_t pwd_size = 64;
//     char pwd_buff[pwd_size];
//     if (get_wifi_pwd(settings_buff, ssid, pwd_buff, pwd_size)) {
//         // TODO
//     }
//     return false;
// }

// bool wifi_scan_connect(char* settings_buff)
// {

//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
//     assert(sta_netif);

//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

//     const int default_scan_list_size = 3;
//     uint16_t number = default_scan_list_size;
//     wifi_ap_record_t ap_info[default_scan_list_size];
//     uint16_t ap_count = 0;
//     memset(ap_info, 0, sizeof(ap_info));

//     ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//     ESP_ERROR_CHECK(esp_wifi_start());
//     ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
//     ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
//     ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
//     ESP_LOGI("cansys", "Total APs scanned = %u", ap_count);
//     for (int i = 0; (i < default_scan_list_size) && (i < ap_count); i++) {
//         ESP_LOGI("cansys", "SSID \t\t%s", ap_info[i].ssid);
//         ESP_LOGI("cansys", "RSSI \t\t%d", ap_info[i].rssi);        
//         ESP_LOGI("cansys", "Channel \t\t%d\n", ap_info[i].primary);

//         if (wifi_connect_attempt(settings_buff, (char*)ap_info[i].ssid)) {
//             return true;
//         }
//     }

//     return false;
// }


// void main_motion(nvs_handle_t nvs_handle) {

//     char* settings_buff = retrieve_settings(nvs_handle);
//     validate_settings(settings_buff);

//     if (!wifi_scan_connect(settings_buff)) ERROR("WIFI connect error");

//     free(settings_buff);
// }

/****************** MAIN *****************/

void app_main()
{
    stdio_init();
    //nvs_handle_t nvs_handle;
    // nvs_init(&nvs_handle);
    // gpio_pins_init();

    // int motion_button_state = !gpio_get_level(GPIO_NUM_12);
    // int camera_button_state = !gpio_get_level(GPIO_NUM_13);

    // if (motion_button_state) main_motion(nvs_handle); //wifi_websocket_client_app_start(motion_sensor_setup, motion_sensor_loop, motion_sensor_data, motion_sensor_connected, motion_sensor_disconnected, motion_sensor_error);
    // else if (camera_button_state) printf("CAMERA MODE...\n"); //wifi_websocket_client_app_start(camera_recorder_setup, camera_recorder_loop, camera_recorder_data, camera_recorder_connected, camera_recorder_disconnected, camera_recorder_error);
    // else main_settings(nvs_handle);
        
        main_settings();

    // nvs_close(nvs_handle);
    stop("LEAVING");
}
