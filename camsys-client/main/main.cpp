#include "esp_err.h"
#include "esp_vfs_dev.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_tls.h"
#include "esp_http_client.h"
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

const size_t print_buffer_size = 1000;

void printlnf(const char* fmt, ...) {
    char buffer[print_buffer_size] = {0};
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    println(buffer);
}

bool errorlnf(int line, const char* fmt, ...) {
    char buffer[print_buffer_size] = {0};
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    errorln(line, buffer);
    esp_restart();

    return false;
}

void debuglnf(int line, const char* fmt, ...) {
    char buffer[print_buffer_size] = {0};
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

char* reads(char* buff, size_t size) {
    //char* buff = (char*)malloc(sizeof(char) * size);
    //if (!buff) ERRORF("Memory allocation of %ld chars error.", size);
    if (!fgets(buff, size, stdin)) ERROR("Read error.");
    buff[strlen(buff)-1] = '\0';
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

uint16_t nvs_getu16(nvs_handle_t nvs_handle, const char* key, uint16_t default_value) {
    uint16_t out_value = default_value;
    esp_err_t err = nvs_get_u16(nvs_handle, key, &out_value);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) ESP_ERROR_CHECK(err);
    return out_value;
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

void nvs_setu16(nvs_handle_t nvs_handle, const char* key, uint16_t value) {
    esp_err_t err = nvs_set_u16(nvs_handle, key, value);
    if (err != ESP_OK) nvs_close(nvs_handle);
    ESP_ERROR_CHECK(err);
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


bool settings_validate_uids(char* value, bool halt) {
    if (NULL == value || value[0] == '\0') return halt ? ERROR("Value is missing.") : false;
    if (strlen(value) != 32) return halt ? ERROR("Incorrect UIDS length.") : false;
    return true;
}

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

bool settings_validate_required(char* value, bool halt) {
    if (NULL == value || value[0] == '\0') return halt ? ERROR("Value is missing.") : false;
    return true;
}

bool settings_validate_numeric(char* value, bool halt) {
    size_t len = strlen(value);
    for (size_t i=0; i<len; i++) {
        if (value[i] < '0' || value[i] > '9') return halt ? ERROR("Value is not numeric.") : false;
    }
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
    PRINT("WAITING FOR CAMSYS SETTINGS...");

    bool finish = false;

    const size_t reads_size = 500;
    char input[reads_size] = {0};
    char value[reads_size] = {0};

    while (!finish) {
        input[0] = 0;
        reads(input, reads_size);
        
        if (!strcmp(input, "UID STRING:")) {

            reads(value, reads_size);

            if (!settings_validate_uids(value, false)) {
                PRINT("Invalid UIDS format.");
            } else {
                nvs_sets(nvs_handle, "uids", value);
                PRINT("UIDS accepted.");
            }

        } else if (!strcmp(input, "WIFI CREDENTIALS:")) {

            reads(value, reads_size);

            if (!settings_validate_wifi_credentials(value, false)) {
                PRINT("Invalid WiFi credentials format.");
            } else {
                nvs_sets(nvs_handle, "wifi", value);
                PRINT("Wifi credentials are accepted.");
            }

        } else if (!strcmp(input, "HOST ADDRESS OR IP:")) {

            reads(value, reads_size);

            if (!settings_validate_required(value, false)) {
                PRINT("Invalid host address format.");
            } else {
                nvs_sets(nvs_handle, "host", value);
                PRINT("Host address is accepted.");
            }

        } else if (!strcmp(input, "SECRET:")) {

            reads(value, reads_size);

            if (!settings_validate_required(value, false)) {
                PRINT("Invalid secret format.");
            } else {
                nvs_sets(nvs_handle, "secret", value);
                PRINT("Secret is accepted.");
            }

        } else if (!strcmp(input, "HTTP PREFIX:")) {

            reads(value, reads_size);

            if (!settings_validate_required(value, false)) {
                PRINT("Invalid http prefix format.");
            } else {
                nvs_sets(nvs_handle, "http_prefix", value);
                PRINT("Http prefix is accepted.");
            }

        } else if (!strcmp(input, "HTTP PORT:")) {

            reads(value, reads_size);

            if (!settings_validate_required(value, false) || !settings_validate_numeric(value, false) ) {
                PRINT("Invalid http port format.");
            } else {
                uint16_t value_u16 = atoi(value);
                nvs_setu16(nvs_handle, "http_prefix", value_u16);
                PRINT("Http port is accepted.");
            }

        } else if (!strcmp(input, "COMMIT")) {

            finish = true;
            nvs_store(nvs_handle);
            PRINT("Settings are saved.");

        } else {
            PRINTF("Wrong settings argument: (%s)", input);
        }

    }



    PRINT("FINISHED");
}


/****************** WIFI *****************/


bool wifi_connected = false;
static ip4_addr_t wifi_ip_addr;

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    //PRINTF("WIFI: ***EVENT (%ld)", event_id);

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        //PRINT("WIFI: GOT IP");
        wifi_connected = true;
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        memcpy(&wifi_ip_addr, &event->ip_info.ip, sizeof(wifi_ip_addr));
        ESP_ERROR_CHECK(esp_wifi_scan_stop());
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        //PRINT("WIFI: DISCONNECTED");
        wifi_connected = false;
        // int wifi_reconnect_attempts = 30;
        // while (!wifi_connected && wifi_reconnect_attempts) {
        //     wifi_reconnect_attempts--;
        //     PRINTF("WIFI: RECONNECT ATTEMPTS: %d", wifi_reconnect_attempts);
        //     esp_wifi_connect();
        //     vTaskDelay(3000 / portTICK_RATE_MS);
        // }
        esp_restart();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        //PRINT("WIFI: CONNECTED");
    }
}

bool wifi_connect(const char* ssid, const char* pwd) {

    wifi_config_t wifi_config;
    //wifi_config.sta.ssid[0] = '\0';
    //wifi_config.sta.password[0] = '\0';
    wifi_config.sta.bssid_set = 0;
    wifi_config.sta.pmf_cfg.required = false;

    strncpy((char*)wifi_config.sta.ssid, ssid, 32);
    strncpy((char*)wifi_config.sta.password, pwd, 64);
    
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
    
    //vTaskDelay(3000 / portTICK_RATE_MS);
    while (!wifi_connected) vTaskDelay(300 / portTICK_RATE_MS); //return false;

    PRINTF("Connected to %s", ssid);
    PRINTF("IPv4 address: " IPSTR, IP2STR(&wifi_ip_addr));
    return true;
}

bool wifi_connect_attempt(char* wifi_settings, char* ssid) {
    size_t pwd_size = 64;
    char pwd_buff[pwd_size];
    if (get_wifi_pwd(wifi_settings, ssid, pwd_buff, pwd_size)) {
        // TODO connect...
        //PRINTF("Attempt to connect to '%s'...", ssid);
        return wifi_connect(ssid, pwd_buff);
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

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL) );

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
    ESP_LOGI("camsys", "Total APs scanned = %u", ap_count);
    for (int i = 0; (i < default_scan_list_size) && (i < ap_count); i++) {
        ESP_LOGI("camsys", "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI("camsys", "RSSI \t\t%d", ap_info[i].rssi);        
        ESP_LOGI("camsys", "Channel \t\t%d\n", ap_info[i].primary);

        if (wifi_connect_attempt(wifi_settings, (char*)ap_info[i].ssid)) {
            return true;
        }
    }

    return false;
}

char* app_wifi_start(nvs_handle_t nvs_handle) {
    char* wifi_settings = nvs_gets(nvs_handle, "wifi");
    settings_validate_wifi_credentials(wifi_settings, true);
    if (!wifi_scan_connect(wifi_settings)) ERROR("WIFI connect error");
    return wifi_settings;
}

/****************** MOTION MODE *****************/

typedef void (*http_response_handler_t)(char* buff, size_t size);

http_response_handler_t _http_response_handler = NULL;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD("camsys", "HTTP_EVENT_ERROR");
            PRINT("HTTP ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD("camsys", "HTTP_EVENT_ON_CONNECTED");
            PRINT("HTTP ON CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD("camsys", "HTTP_EVENT_HEADER_SENT");
            PRINT("HTTP HEADER SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD("camsys", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            PRINT("HTTP ON HEADER");
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD("camsys", "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            PRINT("HTTP ON DATA");
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                if (evt->user_data) {
                    memcpy((char*)evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) calloc(esp_http_client_get_content_length(evt->client) + 1, sizeof(char));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE("camsys", "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD("camsys", "HTTP_EVENT_ON_FINISH");
            PRINT("HTTP ON FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX("camsys", output_buffer, output_len);
                if (_http_response_handler) _http_response_handler(output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI("camsys", "HTTP_EVENT_DISCONNECTED");
            PRINT("HTTP DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                if (output_buffer != NULL) {
                    free(output_buffer);
                    output_buffer = NULL;
                }
                output_len = 0;
                ESP_LOGI("camsys", "Last esp error code: 0x%x", err);
                ESP_LOGI("camsys", "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
    }
    return ESP_OK;
}

// TODO: may it should be in the esp-idf lib folder as a macro just like `wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();`, (see at esp_wifi.h) later on mey I will create a pull request?

#define HTTP_CLIENT_CONFIG_DEFAULT(URL) {\
    .url = URL,\
    .host = NULL,\
    .port = 0,\
    .username = NULL,\
    .password = NULL,\
    .auth_type = HTTP_AUTH_TYPE_NONE,\
    .path = NULL,\
    .query = NULL,\
    .cert_pem = NULL,\
    .client_cert_pem = NULL,\
    .client_key_pem = NULL,\
    .method = HTTP_METHOD_GET,\
    .timeout_ms = 0,\
    .disable_auto_redirect = false,\
    .max_redirection_count = 0,\
    .max_authorization_retries = 0,\
    .event_handler = NULL,\
    .transport_type = HTTP_TRANSPORT_UNKNOWN,\
    .buffer_size = 0,\
    .buffer_size_tx = 0,\
    .user_data = NULL,\
    .is_async = false,\
    .use_global_ca_store = false,\
    .skip_cert_common_name_check = false,\
};


bool http_request(
    esp_http_client_method_t method, const char* url, const char* post_data,
    http_response_handler_t response_handler
) {
    esp_http_client_config_t config = HTTP_CLIENT_CONFIG_DEFAULT(url);
    config.method = method;
    config.event_handler = _http_event_handler;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, method);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    

    PRINT("HTTP PERFOM...");
    _http_response_handler = response_handler;
    esp_err_t err = esp_http_client_perform(client);
    PRINT("HTTP PERFORMED.");
    if (err == ESP_OK) {
        ESP_LOGI("camsys", "HTTP POST Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE("camsys", "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err == ESP_OK;
}

void app_http_response_handler(char* buff, size_t size) {
    PRINTF("RESPONSE STRING LENGTH: %d, BUFF_SIZE: %d", strlen(buff), size);
}

bool app_http_request_join(nvs_handle_t nvs_handle) {
    const size_t strmax = 255;
    char url[strmax];
    char post_data[strmax];

    char* uids = nvs_gets(nvs_handle, "uids"); // TODO add to settings
    char* host = nvs_gets(nvs_handle, "host");
    char* http_prefix = nvs_gets(nvs_handle, "http_prefix"); // TODO add to settings
    uint16_t http_port = nvs_getu16(nvs_handle, "http_port", 3000); // TODO add to settings
    char* secret = nvs_gets(nvs_handle, "secret");

    snprintf(url, strmax, "%s://%s:%d/join", http_prefix, host, http_port);
    snprintf(post_data, strmax, "client=%s&secret=%s", uids, secret);

    bool ret = http_request(HTTP_METHOD_POST, url, post_data, app_http_response_handler);

    free(secret);
    free(http_prefix);
    free(host);
    free(uids);

    return ret;
}

void app_main_motion(nvs_handle_t nvs_handle) {
    char* wifi_settings = app_wifi_start(nvs_handle);
    
    app_http_request_join(nvs_handle);

    int counter = 0;
    while(1) {
        PRINTF("counter:%d", counter++);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }


    free(wifi_settings);
}

/****************** MAIN *****************/


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
