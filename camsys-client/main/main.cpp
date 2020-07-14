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


#define PRINT(msg) println(msg)
#define ERROR(msg) errorln(__LINE__, msg)
#define DEBUG(msg) debugln(__LINE__, msg)

#define PRINTF(fmt, ...) printlnf(fmt, __VA_ARGS__)
#define ERRORF(fmt, ...) errorlnf(__LINE__, fmt, __VA_ARGS__)
#define DEBUGF(fmt, ...) debuglnf(__LINE__, fmt, __VA_ARGS__)

// void println(const char* msg) {
//     printf("%s\n", msg ? msg : "");
// }

void println(const char* msg) {
    printf("%s\n", msg);
}

bool errorln(int line, const char* msg) {
    printf("ERROR at line %d: %s\n", line, msg);
    return false;
}

void debugln(int line, const char* msg) {
    printf("DEBUG at line %d: %s", line, msg);
}

void printlnf(const char* fmt, ...) {
    char buffer[4096] = {0};
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    println(buffer);
}

bool errorlnf(int line, const char* fmt, ...) {
    char buffer[4096] = {0};
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    errorln(line, buffer);
    esp_restart();

    return false;
}

void debuglnf(int line, const char* fmt, ...) {
    char buffer[4096] = {0};
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    debugln(line, buffer);
}

void stop(const char* msg) {
    PRINT(msg);
    esp_restart();
}

const size_t reads_size = 1000;

char* reads(size_t size) {
    char* buff = (char*)malloc(sizeof(char) * size);
    if (!buff) ERRORF("Memory allocation of %ld chars error.", size);
    if (!fgets(buff, size, stdin)) ERROR("Read error.");
    int i=0;
    while (buff[i] != '\n' && buff[i] != '\r' && buff[i] != '\0') i++;
    buff[i] = '\0';
    return buff;
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
    if (setvbuf(stdin, NULL, _IONBF, 0)) ERROR("Initialize VFS & UART error.");
    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install( (uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0) );
    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
}

/****************** NVS *****************/

void nvs_init(nvs_handle_t* nvs_handle) {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    ESP_ERROR_CHECK( nvs_open("storage", NVS_READWRITE, nvs_handle) );
}

char* nvs_gets(nvs_handle_t nvs_handle, const char* key) {
    size_t stored_value_size = 100;
    char* value_buff = (char*)malloc(sizeof(char) * stored_value_size);
    if (!value_buff) ERROR("Memory allocation error.");
    esp_err_t err = nvs_get_str(nvs_handle, key, NULL, &stored_value_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        value_buff[0] = '\0';
        return value_buff;
    } else if (err == ESP_OK) {
        value_buff = (char*)realloc(value_buff, sizeof(char) * stored_value_size);
        if (!value_buff) ERRORF("Memory of %ld chars error.", stored_value_size);
        err = nvs_get_str(nvs_handle, key, value_buff, &stored_value_size);
    }
    ESP_ERROR_CHECK(err);
    return value_buff;
}

void nvs_sets(nvs_handle_t nvs_handle, const char* key, const char* valstr) {
    esp_err_t err = nvs_set_str(nvs_handle, key, valstr);
    if (err != ESP_OK) nvs_close(nvs_handle);
    ESP_ERROR_CHECK(err);
}

void nvs_store(nvs_handle_t nvs_handle) {
    ESP_ERROR_CHECK(nvs_commit(nvs_handle));
}

// --------- NVS SETTINGS ---------


// --------

bool settings_validate_wifi_credentials(char* value, bool halt) {
    if (NULL == value || value[0] == '\0')
        return halt ? ERROR("WIFI credential settings are missing.") : false;
    int num = get_str_pieces(value, ';');
    for (int i=0; i<num; i++) {
        size_t size = 100;
        char buff[size];
        get_str_piece_at(buff, size, value, ';', i);
        if (get_str_pieces(buff, ':') < 2) 
            return halt ? ERROR("WIFI credential settings is incorrect.") : false;
    }
    return true;
}

bool settings_validate_host(char* value, bool halt) {
    if (NULL == value || value[0] == '\0') return halt ? ERROR("Host settings are missing.") : false;
    return true;
}

// ------

void get_wifi_credential_from_settings_at(
    char* wifi_settings, 
    char* ssid_buff, size_t ssid_size, 
    char* pwd_buff, size_t pwd_size, 
    size_t at) 
{
    size_t wifi_credential_size = 100;
    char wifi_credential_buff[wifi_credential_size];
    get_str_piece_at(wifi_credential_buff, wifi_credential_size, wifi_settings, ';', at);

    get_str_piece_at(ssid_buff, ssid_size, wifi_credential_buff, ':', 0);
    get_str_piece_at(pwd_buff, pwd_size, wifi_credential_buff, ':', 1); 

}

bool get_wifi_pwd(char* wifi_settings, const char* ssid, char* pwd_buff, size_t pwd_size) {

    size_t ssid_size = 32;
    char ssid_buff[ssid_size];

    size_t wifi_credentials_num = get_str_pieces(wifi_settings, ';');
    for (int i=0; i<wifi_credentials_num; i++) {
        get_wifi_credential_from_settings_at(wifi_settings, ssid_buff, ssid_size, pwd_buff, pwd_size, i);

        if (!strcmp(ssid, ssid_buff)) return true;
    }

    pwd_size = 0;
    pwd_buff[0] = '\0';

    return false;
}

/****************** GPIO SETUP *****************/

void gpio_pins_init() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL<<GPIO_NUM_12) | (1ULL<<GPIO_NUM_13),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK( gpio_config(&io_conf) );
}

/****************** SETTINGS MODE *****************/

void app_main_settings(nvs_handle_t nvs_handle)
{
    //PRINT("WAITING FOR CAMSYS SETTINGS...");

    bool finish = false;

    int i=0, log_size = 100;
    char* log[log_size];

    while (!finish) {
        char* input = reads(reads_size);
        char* value = reads(reads_size);
        
        if (!strcmp(input, "WIFI CREDENTIALS:")) {

            if (!settings_validate_wifi_credentials(value, false)) {
                PRINT("Invalid WiFi credentials format.");
            } else {
                nvs_sets(nvs_handle, "wifi", value);
                PRINT("Wifi credentials are accepted.");
            }

        } else if (!strcmp(input, "HOST ADDRESS OR IP:")) {

            if (!settings_validate_host(value, false)) {
                PRINT("Invalid host address format.");
            } else {
                nvs_sets(nvs_handle, "host", value);
                PRINT("Host address is accepted.");
            }

        } else if (!strcmp(input, "COMMIT")) {

            finish = true;
            nvs_store(nvs_handle);
            PRINT("Settings are saved.");

        } else {
            PRINTF("Wrong settings argument: '%s'", input);
        }

        log[i] = value; i++; 
        log[i] = input; i++;

        if (i>=log_size-2) finish = true;
    }

    for (int j=0; j<i; j++) free(log[j]);


    PRINT("FINISHED");
}

/****************** MOTION MODE *****************/





bool wifi_connect_attempt(char* wifi_settings, char* ssid) {
    size_t pwd_size = 64;
    char pwd_buff[pwd_size];
    if (get_wifi_pwd(wifi_settings, ssid, pwd_buff, pwd_size)) {
        // TODO connect...
    }
    return false;
}

bool wifi_scan_connect(char* wifi_settings)
{

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    const int default_scan_list_size = 3;
    uint16_t number = default_scan_list_size;
    wifi_ap_record_t ap_info[default_scan_list_size];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI("cansys", "Total APs scanned = %u", ap_count);
    for (int i = 0; (i < default_scan_list_size) && (i < ap_count); i++) {
        ESP_LOGI("cansys", "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI("cansys", "RSSI \t\t%d", ap_info[i].rssi);        
        ESP_LOGI("cansys", "Channel \t\t%d\n", ap_info[i].primary);

        if (wifi_connect_attempt(wifi_settings, (char*)ap_info[i].ssid)) {
            return true;
        }
    }

    return false;
}


void app_main_motion(nvs_handle_t nvs_handle) {
    char* wifi_settings = nvs_gets(nvs_handle, "wifi");
    settings_validate_wifi_credentials(wifi_settings, true);

    if (!wifi_scan_connect(wifi_settings)) ERROR("WIFI connect error");

    free(wifi_settings);
}

/****************** MAIN *****************/

class Camsys {
    
};


extern "C" void app_main()
{
    stdio_init();

    nvs_handle_t nvs_handle;
    nvs_init(&nvs_handle);
    gpio_pins_init();

    int motion_button_state = !gpio_get_level(GPIO_NUM_12);
    int camera_button_state = !gpio_get_level(GPIO_NUM_13);

    if (motion_button_state) 
        app_main_motion(nvs_handle); //wifi_websocket_client_app_start(motion_sensor_setup, motion_sensor_loop, motion_sensor_data, motion_sensor_connected, motion_sensor_disconnected, motion_sensor_error);
    else if (camera_button_state) 
        printf("CAMERA MODE...\n"); //wifi_websocket_client_app_start(camera_recorder_setup, camera_recorder_loop, camera_recorder_data, camera_recorder_connected, camera_recorder_disconnected, camera_recorder_error);
    else 
        app_main_settings(nvs_handle);

    nvs_close(nvs_handle);
    stop("LEAVING");
}
