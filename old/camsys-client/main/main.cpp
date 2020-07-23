// #include "esp_err.h"
// #include "esp_vfs_dev.h"
// #include "esp_wifi.h"
// #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
// #include "esp_log.h"
// #include "esp_event.h"
// #include "esp_tls.h"
// #include "esp_http_client.h"
// #include "driver/uart.h"
// #include "driver/gpio.h"
// #include "nvs_flash.h"
// #include "nvs.h"
// #include "sdkconfig.h"

// #include "esp_camera.h"
// #include "xclk.h"
// #include "sccb.h"
// #include "ov2640.h"
// #include "sensor.h"
// #include "yuv.h"
// #include "twi.h"
// #include "esp_jpg_decode.h"

// static const char *TAG = "espcam";
// #include "xclk.c"
// #include "ov2640.c"
// #include "sccb.c"
// #include "sensor.c"
// #include "yuv.c"
// #include "twi.c"
// #include "to_bmp.c"
// #include "esp_jpg_decode.c"
// #include "camera.c"


// /****************** COMMON *****************/

// // -------- IO ---------


// #define PRINT(msg) println(msg)
// #define ERROR(msg) errorln(__LINE__, msg)
// #define DEBUG(msg) debugln(__LINE__, msg)

// #define PRINTF(fmt, ...) printlnf(fmt, __VA_ARGS__)
// #define ERRORF(fmt, ...) errorlnf(__LINE__, fmt, __VA_ARGS__)
// #define DEBUGF(fmt, ...) debuglnf(__LINE__, fmt, __VA_ARGS__)

// // void println(const char* msg) {
// //     printf("%s\n", msg ? msg : "");
// // }

// void println(const char* msg) {
//     printf("%s\n", msg);
// }

// bool errorln(int line, const char* msg) {
//     printf("ERROR at line %d: %s\n", line, msg);
//     return false;
// }

// void debugln(int line, const char* msg) {
//     printf("DEBUG at line %d: %s", line, msg);
// }

// const size_t print_buffer_size = 1000;

// void printlnf(const char* fmt, ...) {
//     char buffer[print_buffer_size] = {0};
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buffer, sizeof(buffer), fmt, args);
//     va_end(args);

//     println(buffer);
// }

// bool errorlnf(int line, const char* fmt, ...) {
//     char buffer[print_buffer_size] = {0};
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buffer, sizeof(buffer), fmt, args);
//     va_end(args);

//     errorln(line, buffer);
//     esp_restart();

//     return false;
// }

// void debuglnf(int line, const char* fmt, ...) {
//     char buffer[print_buffer_size] = {0};
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buffer, sizeof(buffer), fmt, args);
//     va_end(args);

//     debugln(line, buffer);
// }

// void stop(const char* msg) {
//     PRINT(msg);
//     esp_restart();
// }

// char* reads(char* buff, size_t size) {
//     //char* buff = (char*)malloc(sizeof(char) * size);
//     //if (!buff) ERRORF("Memory allocation of %ld chars error.", size);
//     if (!fgets(buff, size, stdin)) ERROR("Read error.");
//     buff[strlen(buff)-1] = '\0';
//     return buff;
// }


// // -------- STRING ---------

// int get_str_pieces(const char* str, char delim) {
//     int i=0, pieces = 1;
//     while(str[i]) {
//         if(str[i] == delim) pieces++;
//         i++;
//     }
//     return pieces;
// }

// int get_str_piece_at(char* buff, size_t size, const char* str, char delim, int pos) {
//     int i=0, j=0, p = 0;
//     buff[j] = 0;
//     while(str && str[i]) {
//         if (p == pos) {     
//             if(str[i] == delim) break;
//             buff[j] = str[i];
//             if (size <= j) break;
//             j++;
//         }
//         if(str[i] == delim) p++;
//         i++;
//     }
//     buff[j] = 0;
//     return j;
// }

// /****************** STDIO *****************/

// void stdio_init(void)
// {
//     // Initialize VFS & UART so we can use std::cout/cin
//     if (setvbuf(stdin, NULL, _IONBF, 0)) ERROR("Initialize VFS & UART error.");
//     /* Install UART driver for interrupt-driven reads and writes */
//     ESP_ERROR_CHECK( uart_driver_install( (uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0) );
//     /* Tell VFS to use UART driver */
//     esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
//     esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
//     /* Move the caret to the beginning of the next line on '\n' */
//     esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
// }

// /****************** NVS *****************/

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

// uint16_t nvs_getu16(nvs_handle_t nvs_handle, const char* key, uint16_t default_value) {
//     uint16_t out_value = default_value;
//     esp_err_t err = nvs_get_u16(nvs_handle, key, &out_value);
//     if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) ESP_ERROR_CHECK(err);
//     return out_value;
// }

// char* nvs_gets(nvs_handle_t nvs_handle, const char* key) {
//     size_t stored_value_size = 100;
//     char* value_buff = (char*)malloc(sizeof(char) * stored_value_size);
//     if (!value_buff) ERROR("Memory allocation error.");
//     esp_err_t err = nvs_get_str(nvs_handle, key, NULL, &stored_value_size);
//     if (err == ESP_ERR_NVS_NOT_FOUND) {
//         value_buff[0] = '\0';
//         return value_buff;
//     } else if (err == ESP_OK) {
//         value_buff = (char*)realloc(value_buff, sizeof(char) * stored_value_size);
//         if (!value_buff) ERRORF("Memory of %ld chars error.", stored_value_size);
//         err = nvs_get_str(nvs_handle, key, value_buff, &stored_value_size);
//     }
//     ESP_ERROR_CHECK(err);
//     return value_buff;
// }

// void nvs_setu16(nvs_handle_t nvs_handle, const char* key, uint16_t value) {
//     esp_err_t err = nvs_set_u16(nvs_handle, key, value);
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


// // --------


// bool settings_validate_uids(char* value, bool halt) {
//     if (NULL == value || value[0] == '\0') return halt ? ERROR("Value is missing.") : false;
//     if (strlen(value) != 32) return halt ? ERROR("Incorrect UIDS length.") : false;
//     return true;
// }

// bool settings_validate_wifi_credentials(char* value, bool halt) {
//     if (NULL == value || value[0] == '\0')
//         return halt ? ERROR("WIFI credential settings are missing.") : false;
//     int num = get_str_pieces(value, ';');
//     for (int i=0; i<num; i++) {
//         size_t size = 100;
//         char buff[size];
//         get_str_piece_at(buff, size, value, ';', i);
//         if (get_str_pieces(buff, ':') < 2) 
//             return halt ? ERROR("WIFI credential settings is incorrect.") : false;
//     }
//     return true;
// }

// bool settings_validate_required(char* value, bool halt) {
//     if (NULL == value || value[0] == '\0') return halt ? ERROR("Value is missing.") : false;
//     return true;
// }

// bool settings_validate_numeric(char* value, bool halt) {
//     size_t len = strlen(value);
//     for (size_t i=0; i<len; i++) {
//         if (value[i] < '0' || value[i] > '9') return halt ? ERROR("Value is not numeric.") : false;
//     }
//     return true;
// }

// // ------

// void get_wifi_credential_from_settings_at(
//     char* wifi_settings, 
//     char* ssid_buff, size_t ssid_size, 
//     char* pwd_buff, size_t pwd_size, 
//     size_t at) 
// {
//     size_t wifi_credential_size = 100;
//     char wifi_credential_buff[wifi_credential_size];
//     get_str_piece_at(wifi_credential_buff, wifi_credential_size, wifi_settings, ';', at);

//     get_str_piece_at(ssid_buff, ssid_size, wifi_credential_buff, ':', 0);
//     get_str_piece_at(pwd_buff, pwd_size, wifi_credential_buff, ':', 1); 

// }

// bool get_wifi_pwd(char* wifi_settings, const char* ssid, char* pwd_buff, size_t pwd_size) {

//     size_t ssid_size = 32;
//     char ssid_buff[ssid_size];

//     size_t wifi_credentials_num = get_str_pieces(wifi_settings, ';');
//     for (int i=0; i<wifi_credentials_num; i++) {
//         get_wifi_credential_from_settings_at(wifi_settings, ssid_buff, ssid_size, pwd_buff, pwd_size, i);

//         if (!strcmp(ssid, ssid_buff)) return true;
//     }

//     pwd_size = 0;
//     pwd_buff[0] = '\0';

//     return false;
// }

// /****************** GPIO SETUP *****************/

// void gpio_pins_init() {
//     gpio_config_t io_conf = {
//         .pin_bit_mask = (1ULL<<GPIO_NUM_12) | (1ULL<<GPIO_NUM_13),
//         .mode = GPIO_MODE_INPUT,
//         .pull_up_en = GPIO_PULLUP_ENABLE,
//         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .intr_type = GPIO_INTR_DISABLE,
//     };
//     ESP_ERROR_CHECK( gpio_config(&io_conf) );
// }

// /****************** SETTINGS MODE *****************/

// void app_main_settings(nvs_handle_t nvs_handle)
// {
//     PRINT("WAITING FOR CAMSYS SETTINGS...");

//     bool finish = false;

//     const size_t reads_size = 500;
//     char input[reads_size] = {0};
//     char value[reads_size] = {0};

//     while (!finish) {
//         input[0] = 0;
//         reads(input, reads_size);
        
//         if (!strcmp(input, "UID STRING:")) {

//             reads(value, reads_size);

//             if (!settings_validate_uids(value, false)) {
//                 PRINT("Invalid UIDS format.");
//             } else {
//                 nvs_sets(nvs_handle, "uids", value);
//                 PRINT("UIDS accepted.");
//             }

//         } else if (!strcmp(input, "WIFI CREDENTIALS:")) {

//             reads(value, reads_size);

//             if (!settings_validate_wifi_credentials(value, false)) {
//                 PRINT("Invalid WiFi credentials format.");
//             } else {
//                 nvs_sets(nvs_handle, "wifi", value);
//                 PRINT("Wifi credentials are accepted.");
//             }

//         } else if (!strcmp(input, "HOST ADDRESS OR IP:")) {

//             reads(value, reads_size);

//             if (!settings_validate_required(value, false)) {
//                 PRINT("Invalid host address format.");
//             } else {
//                 nvs_sets(nvs_handle, "host", value);
//                 PRINT("Host address is accepted.");
//             }

//         } else if (!strcmp(input, "SECRET:")) {

//             reads(value, reads_size);

//             if (!settings_validate_required(value, false)) {
//                 PRINT("Invalid secret format.");
//             } else {
//                 nvs_sets(nvs_handle, "secret", value);
//                 PRINT("Secret is accepted.");
//             }

//         } else if (!strcmp(input, "HTTP PREFIX:")) {

//             reads(value, reads_size);

//             if (!settings_validate_required(value, false)) {
//                 PRINT("Invalid http prefix format.");
//             } else {
//                 nvs_sets(nvs_handle, "http_prefix", value);
//                 PRINT("Http prefix is accepted.");
//             }

//         } else if (!strcmp(input, "HTTP PORT:")) {

//             reads(value, reads_size);

//             if (!settings_validate_required(value, false) || !settings_validate_numeric(value, false) ) {
//                 PRINT("Invalid http port format.");
//             } else {
//                 uint16_t value_u16 = atoi(value);
//                 nvs_setu16(nvs_handle, "http_prefix", value_u16);
//                 PRINT("Http port is accepted.");
//             }

//         } else if (!strcmp(input, "COMMIT")) {

//             finish = true;
//             nvs_store(nvs_handle);
//             PRINT("Settings are saved.");

//         } else {
//             PRINTF("Wrong settings argument: (%s)", input);
//         }

//     }



//     PRINT("FINISHED");
// }


// /****************** WIFI *****************/


// bool wifi_connected = false;
// static ip4_addr_t wifi_ip_addr;

// void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
//     //PRINTF("WIFI: ***EVENT (%ld)", event_id);

//     if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//         //PRINT("WIFI: GOT IP");
//         wifi_connected = true;
//         ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
//         memcpy(&wifi_ip_addr, &event->ip_info.ip, sizeof(wifi_ip_addr));
//         ESP_ERROR_CHECK(esp_wifi_scan_stop());
//     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//         //PRINT("WIFI: DISCONNECTED");
//         wifi_connected = false;
//         int wifi_reconnect_attempts = 30;
//         while (!wifi_connected && wifi_reconnect_attempts) {
//             wifi_reconnect_attempts--;
//             PRINTF("WIFI: RECONNECT ATTEMPTS: %d", wifi_reconnect_attempts);
//             esp_wifi_connect();
//             vTaskDelay(3000 / portTICK_RATE_MS);
//         }
//         if (!wifi_connected) {
//             ERROR("WIFI DISCONNECTED, RESTART...");
//             esp_restart();
//         }
//     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
//         //PRINT("WIFI: CONNECTED");
//     }
// }

// bool wifi_connect(const char* ssid, const char* pwd) {

//     wifi_config_t wifi_config;
//     //wifi_config.sta.ssid[0] = '\0';
//     //wifi_config.sta.password[0] = '\0';
//     wifi_config.sta.bssid_set = 0;
//     wifi_config.sta.pmf_cfg.required = false;

//     strncpy((char*)wifi_config.sta.ssid, ssid, 32);
//     strncpy((char*)wifi_config.sta.password, pwd, 64);
    
//     ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
//     ESP_ERROR_CHECK(esp_wifi_connect());
    
//     //vTaskDelay(3000 / portTICK_RATE_MS);
//     while (!wifi_connected) vTaskDelay(300 / portTICK_RATE_MS); //return false;

//     PRINTF("Connected to %s", ssid);
//     PRINTF("IPv4 address: " IPSTR, IP2STR(&wifi_ip_addr));
//     return true;
// }

// bool wifi_connect_attempt(char* wifi_settings, char* ssid) {
//     size_t pwd_size = 64;
//     char pwd_buff[pwd_size];
//     if (get_wifi_pwd(wifi_settings, ssid, pwd_buff, pwd_size)) {
//         // TODO connect...
//         //PRINTF("Attempt to connect to '%s'...", ssid);
//         return wifi_connect(ssid, pwd_buff);
//     }
//     return false;
// }

// bool wifi_scan_connect(char* wifi_settings)
// {

//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
//     assert(sta_netif);

//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

//     ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL) );
//     ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL) );

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
//     ESP_LOGI("camsys", "Total APs scanned = %u", ap_count);
//     for (int i = 0; (i < default_scan_list_size) && (i < ap_count); i++) {
//         ESP_LOGI("camsys", "SSID \t\t%s", ap_info[i].ssid);
//         ESP_LOGI("camsys", "RSSI \t\t%d", ap_info[i].rssi);        
//         ESP_LOGI("camsys", "Channel \t\t%d\n", ap_info[i].primary);

//         if (wifi_connect_attempt(wifi_settings, (char*)ap_info[i].ssid)) {
//             return true;
//         }
//     }

//     return false;
// }

// char* app_wifi_start(nvs_handle_t nvs_handle) {
//     char* wifi_settings = nvs_gets(nvs_handle, "wifi");
//     settings_validate_wifi_credentials(wifi_settings, true);
//     if (!wifi_scan_connect(wifi_settings)) ERROR("WIFI connect error");
//     return wifi_settings;
// }
// /****************** HTTP CLIENT *****************/

// typedef void (*http_response_handler_t)(char* buff, size_t size);

// http_response_handler_t _http_response_handler = NULL;

// esp_err_t _http_event_handler(esp_http_client_event_t *evt)
// {
//     static char *output_buffer;  // Buffer to store response of http request from event handler
//     static int output_len;       // Stores number of bytes read
//     switch(evt->event_id) {
//         case HTTP_EVENT_ERROR:
//             // ESP_LOGD("camsys", "HTTP_EVENT_ERROR");
//             // PRINT("HTTP ERROR");
//             break;
//         case HTTP_EVENT_ON_CONNECTED:
//             // ESP_LOGD("camsys", "HTTP_EVENT_ON_CONNECTED");
//             // PRINT("HTTP ON CONNECTED");
//             break;
//         case HTTP_EVENT_HEADER_SENT:
//             // ESP_LOGD("camsys", "HTTP_EVENT_HEADER_SENT");
//             // PRINT("HTTP HEADER SENT");
//             break;
//         case HTTP_EVENT_ON_HEADER:
//             // ESP_LOGD("camsys", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
//             // PRINT("HTTP ON HEADER");
//             break;
//         case HTTP_EVENT_ON_DATA:
//             // ESP_LOGD("camsys", "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
//             // PRINT("HTTP ON DATA");
//             /*
//              *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
//              *  However, event handler can also be used in case chunked encoding is used.
//              */
//             if (!esp_http_client_is_chunked_response(evt->client)) {
//                 // If user_data buffer is configured, copy the response into the buffer
//                 if (evt->user_data) {
//                     memcpy((char*)evt->user_data + output_len, evt->data, evt->data_len);
//                 } else {
//                     if (output_buffer == NULL) {
//                         output_buffer = (char *) calloc(esp_http_client_get_content_length(evt->client) + 1, sizeof(char));
//                         output_len = 0;
//                         if (output_buffer == NULL) {
//                             ESP_LOGE("camsys", "Failed to allocate memory for output buffer");
//                             return ESP_FAIL;
//                         }
//                     }
//                     memcpy(output_buffer + output_len, evt->data, evt->data_len);
//                 }
//                 output_len += evt->data_len;
//             }

//             break;
//         case HTTP_EVENT_ON_FINISH:
//             // ESP_LOGD("camsys", "HTTP_EVENT_ON_FINISH");
//             // PRINT("HTTP ON FINISH");
//             if (output_buffer != NULL) {
//                 // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
//                 // ESP_LOG_BUFFER_HEX("camsys", output_buffer, output_len);
//                 if (_http_response_handler) _http_response_handler(output_buffer, output_len);
//                 free(output_buffer);
//                 output_buffer = NULL;
//             }
//             output_len = 0;
//             break;
//         case HTTP_EVENT_DISCONNECTED:
//             // ESP_LOGI("camsys", "HTTP_EVENT_DISCONNECTED");
//             // PRINT("HTTP DISCONNECTED");
//             int mbedtls_err = 0;
//             esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
//             if (err != 0) {
//                 if (output_buffer != NULL) {
//                     free(output_buffer);
//                     output_buffer = NULL;
//                 }
//                 output_len = 0;
//                 ESP_LOGI("camsys", "Last esp error code: 0x%x", err);
//                 ESP_LOGI("camsys", "Last mbedtls failure: 0x%x", mbedtls_err);
//             }
//             break;
//     }
//     return ESP_OK;
// }

// // TODO: may it should be in the esp-idf lib folder as a macro just like `wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();`, (see at esp_wifi.h) later on mey I will create a pull request?

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


// bool http_request(
//     esp_http_client_method_t method, const char* url, const char* post_data,
//     http_response_handler_t response_handler
// ) {
//     esp_http_client_config_t config = HTTP_CLIENT_CONFIG_DEFAULT(url);
//     config.method = method;
//     config.event_handler = _http_event_handler;

//     esp_http_client_handle_t client = esp_http_client_init(&config);
//     esp_http_client_set_method(client, method);
//     esp_http_client_set_post_field(client, post_data, strlen(post_data));
    

//     // PRINT("HTTP PERFOM...");
//     _http_response_handler = response_handler;
//     esp_err_t err = esp_http_client_perform(client);
//     // PRINT("HTTP PERFORMED.");
//     if (err == ESP_OK) {
//         // ESP_LOGI("camsys", "HTTP POST Status = %d, content_length = %d",
//         //         esp_http_client_get_status_code(client),
//         //         esp_http_client_get_content_length(client));
//     } else {
//         ESP_LOGE("camsys", "HTTP POST request failed: %s", esp_err_to_name(err));
//     }

//     esp_http_client_cleanup(client);
//     return err == ESP_OK;
// }

// void app_http_response_handler(char* buff, size_t size) {
//     PRINTF("RESPONSE STRING LENGTH: %d, BUFF_SIZE: %d", strlen(buff), size);
// }


// /****************** APP TASKS STARTER *****************/

// bool app_http_request_join(nvs_handle_t nvs_handle, const char* additional_join_post_data, http_response_handler_t http_response_handler) {
//     const size_t strmax = 255;
//     char url[strmax];
//     char post_data[strmax];

//     char* uids = nvs_gets(nvs_handle, "uids"); // TODO add to settings
//     char* host = nvs_gets(nvs_handle, "host");
//     char* http_prefix = nvs_gets(nvs_handle, "http_prefix"); // TODO add to settings
//     uint16_t http_port = nvs_getu16(nvs_handle, "http_port", 3000); // TODO add to settings
//     char* secret = nvs_gets(nvs_handle, "secret");

//     snprintf(url, strmax, "%s://%s:%d/join", http_prefix, host, http_port);
//     snprintf(post_data, strmax, "client=%s&secret=%s&%s", uids, secret, additional_join_post_data);

//     bool ret = http_request(HTTP_METHOD_POST, url, post_data, http_response_handler);

//     free(secret);
//     free(http_prefix);
//     free(host);
//     free(uids);

//     return ret;
// }

// typedef void (*additional_join_post_data_cb_t)(char* buff, size_t size); 
// typedef void (*app_main_presetup_func_t)(void);

// void app_main_start(
//     nvs_handle_t nvs_handle, 
//     additional_join_post_data_cb_t additional_join_post_data_cb, 
//     app_main_presetup_func_t presetup,
//     TaskFunction_t subtask
// ) {
//     presetup();

//     char* wifi_settings = app_wifi_start(nvs_handle);
    
//     TaskHandle_t xSubTaskHandle = NULL;
//     const size_t subtask_stack_size = 10000;
//     xTaskCreate(subtask, "subtask", subtask_stack_size, NULL, tskIDLE_PRIORITY, &xSubTaskHandle);

//     const size_t additional_join_post_data_size = 1000;
//     char* additional_join_post_data = (char*)calloc(additional_join_post_data_size, sizeof(char));
//     if (!additional_join_post_data) ERROR("Memory allocation error.");

//     while(1) {
//         additional_join_post_data_cb(additional_join_post_data, additional_join_post_data_size);
//         app_http_request_join(nvs_handle, additional_join_post_data, app_http_response_handler);
//         vTaskDelay(3000 / portTICK_RATE_MS);
//     }

//     free(additional_join_post_data);

//     if( xSubTaskHandle != NULL ) vTaskDelete( xSubTaskHandle );
    
//     free(wifi_settings);
// }


// /****************** MOTION MODE *****************/


// //CAMERA_MODEL_AI_THINKER PIN Map
// #define CAM_PIN_PWDN    (gpio_num_t)32 //power down is not used
// #define CAM_PIN_RESET   (gpio_num_t)-1 //software reset will be performed
// #define CAM_PIN_XCLK    (gpio_num_t)0
// #define CAM_PIN_SIOD    (gpio_num_t)26
// #define CAM_PIN_SIOC    (gpio_num_t)27

// #define CAM_PIN_D7      (gpio_num_t)35
// #define CAM_PIN_D6      (gpio_num_t)34
// #define CAM_PIN_D5      (gpio_num_t)39
// #define CAM_PIN_D4      (gpio_num_t)36
// #define CAM_PIN_D3      (gpio_num_t)21
// #define CAM_PIN_D2      (gpio_num_t)19
// #define CAM_PIN_D1      (gpio_num_t)18
// #define CAM_PIN_D0       (gpio_num_t)5
// #define CAM_PIN_VSYNC   (gpio_num_t)25
// #define CAM_PIN_HREF    (gpio_num_t)23
// #define CAM_PIN_PCLK    (gpio_num_t)22

// static camera_config_t motion_camera_config = {
//     .pin_pwdn  = CAM_PIN_PWDN,
//     .pin_reset = CAM_PIN_RESET,
//     .pin_xclk = CAM_PIN_XCLK,
//     .pin_sscb_sda = CAM_PIN_SIOD,
//     .pin_sscb_scl = CAM_PIN_SIOC,

//     .pin_d7 = CAM_PIN_D7,
//     .pin_d6 = CAM_PIN_D6,
//     .pin_d5 = CAM_PIN_D5,
//     .pin_d4 = CAM_PIN_D4,
//     .pin_d3 = CAM_PIN_D3,
//     .pin_d2 = CAM_PIN_D2,
//     .pin_d1 = CAM_PIN_D1,
//     .pin_d0 = CAM_PIN_D0,
//     .pin_vsync = CAM_PIN_VSYNC,
//     .pin_href = CAM_PIN_HREF,
//     .pin_pclk = CAM_PIN_PCLK,

//     //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
//     .xclk_freq_hz = 20000000,
//     .ledc_timer = LEDC_TIMER_0,
//     .ledc_channel = LEDC_CHANNEL_0,

//     //.pixel_format = PIXFORMAT_RGB888, //PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
//     //.frame_size = FRAMESIZE_QVGA, //FRAMESIZE_UXGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

//     .pixel_format = PIXFORMAT_GRAYSCALE, //PIXFORMAT_RGB888, //PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
//     .frame_size = FRAMESIZE_96X96, //FRAMESIZE_QVGA, //FRAMESIZE_UXGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

//     .jpeg_quality = 0, //0-63 lower number means higher quality
//     .fb_count = 1 //if more than one, i2s runs in continuous mode. Use only with JPEG
// };


// camera_fb_t * fb = NULL;


// struct watch_s {
//     int x;
//     int y;
//     int size;
//     int raster;
//     int threshold;
// };

// typedef struct watch_s watch_t;


// void app_main_motion_init() {
    
//     //power up the camera if PWDN pin is defined
//     if(CAM_PIN_PWDN != -1){
//         pinMode(CAM_PIN_PWDN, OUTPUT);
//         digitalWrite(CAM_PIN_PWDN, LOW);
//     }

//     //initialize the camera
//     ESP_ERROR_CHECK( esp_camera_init(&motion_camera_config) );
// }

// void app_main_motion_show_diff(size_t diff_sum) {
//     char spc[] = "]]]]]]]]]]]]]]]]]]]]]]]]]]]]][";
//     char* s = spc; 
//     for (int i=0; i<29; i++) {
//         if (i*10>diff_sum) s[i] = ' ';
//     }
//     PRINTF("%s (%u)", s, diff_sum);
// }

// //----

// void app_main_motion_additional_join_post_data(char* buff, size_t size) {
//     strncpy(buff, "type=motion", size);
// }

// void app_main_motion_setup() {
//     PRINT("CAMERA INIT...");
//     app_main_motion_init();
//     PRINT("CAMERA INIT...OK");
// }

// void app_main_motion_loop(void * pvParameters) {

//     PRINT("START app_main_motion_loop...");

//     watch_t watcher = {43, 43, 10, 5, 100};
//     static const int watcher_size_max = 40;
//     uint8_t prev_buf[40*40*4];

//     static const size_t buff_size = watcher_size_max*watcher_size_max*4;

//     size_t diff_sum_max = 0;


//     int counter = 0;
//     while(1) {
//         PRINTF("counter:%d", counter++);
//         vTaskDelay(1000 / portTICK_RATE_MS);


//         // fb = esp_camera_fb_get();            
//         // if (!fb) PRINT("Motion Camera capture failed");
        
//         // int xfrom = watcher.x - watcher.size;
//         // int xto = watcher.x + watcher.size;
//         // int yfrom = watcher.y - watcher.size;
//         // int yto = watcher.y + watcher.size;
//         // int i=0;
//         // size_t diff_sum = 0;
//         // for (int x=xfrom; x<xto; x+=watcher.raster) {
//         //     for (int y=yfrom; y<yto; y+=watcher.raster) {
//         //         int diff = fb->buf[x+y*fb->width] - prev_buf[i];
//         //         diff_sum += (diff > 0 ? diff : -diff);
//         //         prev_buf[i] = fb->buf[x+y*fb->width];
//         //         i++;
//         //         if (i>=buff_size) {
//         //             PRINT("buff size too large");
//         //             break;
//         //         }
//         //     }
//         // }

//         // if (diff_sum_max < diff_sum) diff_sum_max = diff_sum;
        
//         // app_main_motion_show_diff(diff_sum);
//         // if (diff_sum >= watcher.threshold) {
//         //     PRINT("******************************************************");
//         //     PRINT("*********************** [ALERT] **********************");
//         //     PRINT("******************************************************");
//         // }
        
//         // esp_camera_fb_return(fb);
//     }
// }

// /****************** MAIN *****************/


// extern "C" void app_main()
// {
//     PRINT("EHH?!");
//     app_main_motion_init();
//     PRINT("WHAT?!");

//     // stdio_init();

//     // nvs_handle_t nvs_handle;
//     // nvs_init(&nvs_handle);
//     // gpio_pins_init();

//     // int motion_button_state = !gpio_get_level(GPIO_NUM_12);
//     // int camera_button_state = !gpio_get_level(GPIO_NUM_13);

//     // if (motion_button_state) 
//     //     app_main_start(nvs_handle, app_main_motion_additional_join_post_data, app_main_motion_setup, app_main_motion_loop); //wifi_websocket_client_app_start(motion_sensor_setup, motion_sensor_loop, motion_sensor_data, motion_sensor_connected, motion_sensor_disconnected, motion_sensor_error);
//     // else if (camera_button_state) 
//     //     printf("CAMERA MODE...\n"); //wifi_websocket_client_app_start(camera_recorder_setup, camera_recorder_loop, camera_recorder_data, camera_recorder_connected, camera_recorder_disconnected, camera_recorder_error);
//     // else 
//     //     app_main_settings(nvs_handle);

//     // nvs_close(nvs_handle);
//     // stop("LEAVING");
// }




















// #include "esp_err.h"
// #include "esp_vfs_dev.h"
// #include "esp_wifi.h"
// #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
// #include "esp_log.h"
// #include "esp_event.h"
// #include "esp_tls.h"
// #include "esp_http_client.h"
// #include "driver/uart.h"
// #include "driver/gpio.h"
// #include "nvs_flash.h"
// #include "nvs.h"
// #include "sdkconfig.h"

// #include "esp_camera.h"
// #include "xclk.h"
// #include "sccb.h"
// #include "ov2640.h"
// #include "sensor.h"
// #include "yuv.h"
// #include "twi.h"
// #include "esp_jpg_decode.h"

// static const char *TAG = "espcam";
// #include "xclk.c"
// #include "ov2640.c"
// #include "sccb.c"
// #include "sensor.c"
// #include "yuv.c"
// #include "twi.c"
// #include "to_bmp.c"
// #include "esp_jpg_decode.c"
// #include "camera.c"


// /****************** COMMON *****************/

// // -------- IO ---------


// #define PRINT(msg) println(msg)
// #define ERROR(msg) errorln(__LINE__, msg)
// #define DEBUG(msg) debugln(__LINE__, msg)

// #define PRINTF(fmt, ...) printlnf(fmt, __VA_ARGS__)
// #define ERRORF(fmt, ...) errorlnf(__LINE__, fmt, __VA_ARGS__)
// #define DEBUGF(fmt, ...) debuglnf(__LINE__, fmt, __VA_ARGS__)

// // void println(const char* msg) {
// //     printf("%s\n", msg ? msg : "");
// // }

// void println(const char* msg) {
//     printf("%s\n", msg);
// }

// bool errorln(int line, const char* msg) {
//     printf("ERROR at line %d: %s\n", line, msg);
//     return false;
// }

// void debugln(int line, const char* msg) {
//     printf("DEBUG at line %d: %s", line, msg);
// }

// const size_t print_buffer_size = 1000;

// void printlnf(const char* fmt, ...) {
//     char buffer[print_buffer_size] = {0};
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buffer, sizeof(buffer), fmt, args);
//     va_end(args);

//     println(buffer);
// }

// bool errorlnf(int line, const char* fmt, ...) {
//     char buffer[print_buffer_size] = {0};
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buffer, sizeof(buffer), fmt, args);
//     va_end(args);

//     errorln(line, buffer);
//     esp_restart();

//     return false;
// }

// void debuglnf(int line, const char* fmt, ...) {
//     char buffer[print_buffer_size] = {0};
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buffer, sizeof(buffer), fmt, args);
//     va_end(args);

//     debugln(line, buffer);
// }

// void stop(const char* msg) {
//     PRINT(msg);
//     esp_restart();
// }

// char* reads(char* buff, size_t size) {
//     //char* buff = (char*)malloc(sizeof(char) * size);
//     //if (!buff) ERRORF("Memory allocation of %ld chars error.", size);
//     if (!fgets(buff, size, stdin)) ERROR("Read error.");
//     buff[strlen(buff)-1] = '\0';
//     return buff;
// }


// // -------- STRING ---------

// int get_str_pieces(const char* str, char delim) {
//     int i=0, pieces = 1;
//     while(str[i]) {
//         if(str[i] == delim) pieces++;
//         i++;
//     }
//     return pieces;
// }

// int get_str_piece_at(char* buff, size_t size, const char* str, char delim, int pos) {
//     int i=0, j=0, p = 0;
//     buff[j] = 0;
//     while(str && str[i]) {
//         if (p == pos) {     
//             if(str[i] == delim) break;
//             buff[j] = str[i];
//             if (size <= j) break;
//             j++;
//         }
//         if(str[i] == delim) p++;
//         i++;
//     }
//     buff[j] = 0;
//     return j;
// }

// /****************** STDIO *****************/

// void stdio_init(void)
// {
//     // Initialize VFS & UART so we can use std::cout/cin
//     if (setvbuf(stdin, NULL, _IONBF, 0)) ERROR("Initialize VFS & UART error.");
//     /* Install UART driver for interrupt-driven reads and writes */
//     ESP_ERROR_CHECK( uart_driver_install( (uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0) );
//     /* Tell VFS to use UART driver */
//     esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
//     esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
//     /* Move the caret to the beginning of the next line on '\n' */
//     esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
// }

// /****************** NVS *****************/

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

// uint16_t nvs_getu16(nvs_handle_t nvs_handle, const char* key, uint16_t default_value) {
//     uint16_t out_value = default_value;
//     esp_err_t err = nvs_get_u16(nvs_handle, key, &out_value);
//     if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) ESP_ERROR_CHECK(err);
//     return out_value;
// }

// char* nvs_gets(nvs_handle_t nvs_handle, const char* key) {
//     size_t stored_value_size = 100;
//     char* value_buff = (char*)malloc(sizeof(char) * stored_value_size);
//     if (!value_buff) ERROR("Memory allocation error.");
//     esp_err_t err = nvs_get_str(nvs_handle, key, NULL, &stored_value_size);
//     if (err == ESP_ERR_NVS_NOT_FOUND) {
//         value_buff[0] = '\0';
//         return value_buff;
//     } else if (err == ESP_OK) {
//         value_buff = (char*)realloc(value_buff, sizeof(char) * stored_value_size);
//         if (!value_buff) ERRORF("Memory of %ld chars error.", stored_value_size);
//         err = nvs_get_str(nvs_handle, key, value_buff, &stored_value_size);
//     }
//     ESP_ERROR_CHECK(err);
//     return value_buff;
// }

// void nvs_setu16(nvs_handle_t nvs_handle, const char* key, uint16_t value) {
//     esp_err_t err = nvs_set_u16(nvs_handle, key, value);
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


// // --------


// bool settings_validate_uids(char* value, bool halt) {
//     if (NULL == value || value[0] == '\0') return halt ? ERROR("Value is missing.") : false;
//     if (strlen(value) != 32) return halt ? ERROR("Incorrect UIDS length.") : false;
//     return true;
// }

// bool settings_validate_wifi_credentials(char* value, bool halt) {
//     if (NULL == value || value[0] == '\0')
//         return halt ? ERROR("WIFI credential settings are missing.") : false;
//     int num = get_str_pieces(value, ';');
//     for (int i=0; i<num; i++) {
//         size_t size = 100;
//         char buff[size];
//         get_str_piece_at(buff, size, value, ';', i);
//         if (get_str_pieces(buff, ':') < 2) 
//             return halt ? ERROR("WIFI credential settings is incorrect.") : false;
//     }
//     return true;
// }

// bool settings_validate_required(char* value, bool halt) {
//     if (NULL == value || value[0] == '\0') return halt ? ERROR("Value is missing.") : false;
//     return true;
// }

// bool settings_validate_numeric(char* value, bool halt) {
//     size_t len = strlen(value);
//     for (size_t i=0; i<len; i++) {
//         if (value[i] < '0' || value[i] > '9') return halt ? ERROR("Value is not numeric.") : false;
//     }
//     return true;
// }

// // ------

// void get_wifi_credential_from_settings_at(
//     char* wifi_settings, 
//     char* ssid_buff, size_t ssid_size, 
//     char* pwd_buff, size_t pwd_size, 
//     size_t at) 
// {
//     size_t wifi_credential_size = 100;
//     char wifi_credential_buff[wifi_credential_size];
//     get_str_piece_at(wifi_credential_buff, wifi_credential_size, wifi_settings, ';', at);

//     get_str_piece_at(ssid_buff, ssid_size, wifi_credential_buff, ':', 0);
//     get_str_piece_at(pwd_buff, pwd_size, wifi_credential_buff, ':', 1); 

// }

// bool get_wifi_pwd(char* wifi_settings, const char* ssid, char* pwd_buff, size_t pwd_size) {

//     size_t ssid_size = 32;
//     char ssid_buff[ssid_size];

//     size_t wifi_credentials_num = get_str_pieces(wifi_settings, ';');
//     for (int i=0; i<wifi_credentials_num; i++) {
//         get_wifi_credential_from_settings_at(wifi_settings, ssid_buff, ssid_size, pwd_buff, pwd_size, i);

//         if (!strcmp(ssid, ssid_buff)) return true;
//     }

//     pwd_size = 0;
//     pwd_buff[0] = '\0';

//     return false;
// }

// /****************** GPIO SETUP *****************/

// void gpio_pins_init() {
//     gpio_config_t io_conf = {
//         .pin_bit_mask = (1ULL<<GPIO_NUM_12) | (1ULL<<GPIO_NUM_13),
//         .mode = GPIO_MODE_INPUT,
//         .pull_up_en = GPIO_PULLUP_ENABLE,
//         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .intr_type = GPIO_INTR_DISABLE,
//     };
//     ESP_ERROR_CHECK( gpio_config(&io_conf) );
// }

// /****************** SETTINGS MODE *****************/

// void app_main_settings(nvs_handle_t nvs_handle)
// {
//     PRINT("WAITING FOR CAMSYS SETTINGS...");

//     bool finish = false;

//     const size_t reads_size = 500;
//     char input[reads_size] = {0};
//     char value[reads_size] = {0};

//     while (!finish) {
//         input[0] = 0;
//         reads(input, reads_size);
        
//         if (!strcmp(input, "UID STRING:")) {

//             reads(value, reads_size);

//             if (!settings_validate_uids(value, false)) {
//                 PRINT("Invalid UIDS format.");
//             } else {
//                 nvs_sets(nvs_handle, "uids", value);
//                 PRINT("UIDS accepted.");
//             }

//         } else if (!strcmp(input, "WIFI CREDENTIALS:")) {

//             reads(value, reads_size);

//             if (!settings_validate_wifi_credentials(value, false)) {
//                 PRINT("Invalid WiFi credentials format.");
//             } else {
//                 nvs_sets(nvs_handle, "wifi", value);
//                 PRINT("Wifi credentials are accepted.");
//             }

//         } else if (!strcmp(input, "HOST ADDRESS OR IP:")) {

//             reads(value, reads_size);

//             if (!settings_validate_required(value, false)) {
//                 PRINT("Invalid host address format.");
//             } else {
//                 nvs_sets(nvs_handle, "host", value);
//                 PRINT("Host address is accepted.");
//             }

//         } else if (!strcmp(input, "SECRET:")) {

//             reads(value, reads_size);

//             if (!settings_validate_required(value, false)) {
//                 PRINT("Invalid secret format.");
//             } else {
//                 nvs_sets(nvs_handle, "secret", value);
//                 PRINT("Secret is accepted.");
//             }

//         } else if (!strcmp(input, "HTTP PREFIX:")) {

//             reads(value, reads_size);

//             if (!settings_validate_required(value, false)) {
//                 PRINT("Invalid http prefix format.");
//             } else {
//                 nvs_sets(nvs_handle, "http_prefix", value);
//                 PRINT("Http prefix is accepted.");
//             }

//         } else if (!strcmp(input, "HTTP PORT:")) {

//             reads(value, reads_size);

//             if (!settings_validate_required(value, false) || !settings_validate_numeric(value, false) ) {
//                 PRINT("Invalid http port format.");
//             } else {
//                 uint16_t value_u16 = atoi(value);
//                 nvs_setu16(nvs_handle, "http_prefix", value_u16);
//                 PRINT("Http port is accepted.");
//             }

//         } else if (!strcmp(input, "COMMIT")) {

//             finish = true;
//             nvs_store(nvs_handle);
//             PRINT("Settings are saved.");

//         } else {
//             PRINTF("Wrong settings argument: (%s)", input);
//         }

//     }



//     PRINT("FINISHED");
// }


// /****************** WIFI *****************/


// bool wifi_connected = false;
// static ip4_addr_t wifi_ip_addr;

// void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
//     //PRINTF("WIFI: ***EVENT (%ld)", event_id);

//     if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//         //PRINT("WIFI: GOT IP");
//         wifi_connected = true;
//         ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
//         memcpy(&wifi_ip_addr, &event->ip_info.ip, sizeof(wifi_ip_addr));
//         ESP_ERROR_CHECK(esp_wifi_scan_stop());
//     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//         //PRINT("WIFI: DISCONNECTED");
//         wifi_connected = false;
//         int wifi_reconnect_attempts = 30;
//         while (!wifi_connected && wifi_reconnect_attempts) {
//             wifi_reconnect_attempts--;
//             PRINTF("WIFI: RECONNECT ATTEMPTS: %d", wifi_reconnect_attempts);
//             esp_wifi_connect();
//             vTaskDelay(3000 / portTICK_RATE_MS);
//         }
//         if (!wifi_connected) {
//             ERROR("WIFI DISCONNECTED, RESTART...");
//             esp_restart();
//         }
//     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
//         //PRINT("WIFI: CONNECTED");
//     }
// }

// bool wifi_connect(const char* ssid, const char* pwd) {

//     wifi_config_t wifi_config;
//     //wifi_config.sta.ssid[0] = '\0';
//     //wifi_config.sta.password[0] = '\0';
//     wifi_config.sta.bssid_set = 0;
//     wifi_config.sta.pmf_cfg.required = false;

//     strncpy((char*)wifi_config.sta.ssid, ssid, 32);
//     strncpy((char*)wifi_config.sta.password, pwd, 64);
    
//     ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
//     ESP_ERROR_CHECK(esp_wifi_connect());
    
//     //vTaskDelay(3000 / portTICK_RATE_MS);
//     while (!wifi_connected) vTaskDelay(300 / portTICK_RATE_MS); //return false;

//     PRINTF("Connected to %s", ssid);
//     PRINTF("IPv4 address: " IPSTR, IP2STR(&wifi_ip_addr));
//     return true;
// }

// bool wifi_connect_attempt(char* wifi_settings, char* ssid) {
//     size_t pwd_size = 64;
//     char pwd_buff[pwd_size];
//     if (get_wifi_pwd(wifi_settings, ssid, pwd_buff, pwd_size)) {
//         // TODO connect...
//         //PRINTF("Attempt to connect to '%s'...", ssid);
//         return wifi_connect(ssid, pwd_buff);
//     }
//     return false;
// }

// bool wifi_scan_connect(char* wifi_settings)
// {

//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
//     assert(sta_netif);

//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

//     ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL) );
//     ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL) );

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
//     ESP_LOGI("camsys", "Total APs scanned = %u", ap_count);
//     for (int i = 0; (i < default_scan_list_size) && (i < ap_count); i++) {
//         ESP_LOGI("camsys", "SSID \t\t%s", ap_info[i].ssid);
//         ESP_LOGI("camsys", "RSSI \t\t%d", ap_info[i].rssi);        
//         ESP_LOGI("camsys", "Channel \t\t%d\n", ap_info[i].primary);

//         if (wifi_connect_attempt(wifi_settings, (char*)ap_info[i].ssid)) {
//             return true;
//         }
//     }

//     return false;
// }

// char* app_wifi_start(nvs_handle_t nvs_handle) {
//     char* wifi_settings = nvs_gets(nvs_handle, "wifi");
//     settings_validate_wifi_credentials(wifi_settings, true);
//     if (!wifi_scan_connect(wifi_settings)) ERROR("WIFI connect error");
//     return wifi_settings;
// }
// /****************** HTTP CLIENT *****************/

// typedef void (*http_response_handler_t)(char* buff, size_t size);

// http_response_handler_t _http_response_handler = NULL;

// esp_err_t _http_event_handler(esp_http_client_event_t *evt)
// {
//     static char *output_buffer;  // Buffer to store response of http request from event handler
//     static int output_len;       // Stores number of bytes read
//     switch(evt->event_id) {
//         case HTTP_EVENT_ERROR:
//             // ESP_LOGD("camsys", "HTTP_EVENT_ERROR");
//             // PRINT("HTTP ERROR");
//             break;
//         case HTTP_EVENT_ON_CONNECTED:
//             // ESP_LOGD("camsys", "HTTP_EVENT_ON_CONNECTED");
//             // PRINT("HTTP ON CONNECTED");
//             break;
//         case HTTP_EVENT_HEADER_SENT:
//             // ESP_LOGD("camsys", "HTTP_EVENT_HEADER_SENT");
//             // PRINT("HTTP HEADER SENT");
//             break;
//         case HTTP_EVENT_ON_HEADER:
//             // ESP_LOGD("camsys", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
//             // PRINT("HTTP ON HEADER");
//             break;
//         case HTTP_EVENT_ON_DATA:
//             // ESP_LOGD("camsys", "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
//             // PRINT("HTTP ON DATA");
//             /*
//              *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
//              *  However, event handler can also be used in case chunked encoding is used.
//              */
//             if (!esp_http_client_is_chunked_response(evt->client)) {
//                 // If user_data buffer is configured, copy the response into the buffer
//                 if (evt->user_data) {
//                     memcpy((char*)evt->user_data + output_len, evt->data, evt->data_len);
//                 } else {
//                     if (output_buffer == NULL) {
//                         output_buffer = (char *) calloc(esp_http_client_get_content_length(evt->client) + 1, sizeof(char));
//                         output_len = 0;
//                         if (output_buffer == NULL) {
//                             ESP_LOGE("camsys", "Failed to allocate memory for output buffer");
//                             return ESP_FAIL;
//                         }
//                     }
//                     memcpy(output_buffer + output_len, evt->data, evt->data_len);
//                 }
//                 output_len += evt->data_len;
//             }

//             break;
//         case HTTP_EVENT_ON_FINISH:
//             // ESP_LOGD("camsys", "HTTP_EVENT_ON_FINISH");
//             // PRINT("HTTP ON FINISH");
//             if (output_buffer != NULL) {
//                 // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
//                 // ESP_LOG_BUFFER_HEX("camsys", output_buffer, output_len);
//                 if (_http_response_handler) _http_response_handler(output_buffer, output_len);
//                 free(output_buffer);
//                 output_buffer = NULL;
//             }
//             output_len = 0;
//             break;
//         case HTTP_EVENT_DISCONNECTED:
//             // ESP_LOGI("camsys", "HTTP_EVENT_DISCONNECTED");
//             // PRINT("HTTP DISCONNECTED");
//             int mbedtls_err = 0;
//             esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
//             if (err != 0) {
//                 if (output_buffer != NULL) {
//                     free(output_buffer);
//                     output_buffer = NULL;
//                 }
//                 output_len = 0;
//                 ESP_LOGI("camsys", "Last esp error code: 0x%x", err);
//                 ESP_LOGI("camsys", "Last mbedtls failure: 0x%x", mbedtls_err);
//             }
//             break;
//     }
//     return ESP_OK;
// }

// // TODO: may it should be in the esp-idf lib folder as a macro just like `wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();`, (see at esp_wifi.h) later on mey I will create a pull request?

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


// bool http_request(
//     esp_http_client_method_t method, const char* url, const char* post_data,
//     http_response_handler_t response_handler
// ) {
//     esp_http_client_config_t config = HTTP_CLIENT_CONFIG_DEFAULT(url);
//     config.method = method;
//     config.event_handler = _http_event_handler;

//     esp_http_client_handle_t client = esp_http_client_init(&config);
//     esp_http_client_set_method(client, method);
//     esp_http_client_set_post_field(client, post_data, strlen(post_data));
    

//     // PRINT("HTTP PERFOM...");
//     _http_response_handler = response_handler;
//     esp_err_t err = esp_http_client_perform(client);
//     // PRINT("HTTP PERFORMED.");
//     if (err == ESP_OK) {
//         // ESP_LOGI("camsys", "HTTP POST Status = %d, content_length = %d",
//         //         esp_http_client_get_status_code(client),
//         //         esp_http_client_get_content_length(client));
//     } else {
//         ESP_LOGE("camsys", "HTTP POST request failed: %s", esp_err_to_name(err));
//     }

//     esp_http_client_cleanup(client);
//     return err == ESP_OK;
// }

// void app_http_response_handler(char* buff, size_t size) {
//     PRINTF("RESPONSE STRING LENGTH: %d, BUFF_SIZE: %d", strlen(buff), size);
// }


// /****************** APP TASKS STARTER *****************/

// bool app_http_request_join(nvs_handle_t nvs_handle, const char* additional_join_post_data, http_response_handler_t http_response_handler) {
//     const size_t strmax = 255;
//     char url[strmax];
//     char post_data[strmax];

//     char* uids = nvs_gets(nvs_handle, "uids"); // TODO add to settings
//     char* host = nvs_gets(nvs_handle, "host");
//     char* http_prefix = nvs_gets(nvs_handle, "http_prefix"); // TODO add to settings
//     uint16_t http_port = nvs_getu16(nvs_handle, "http_port", 3000); // TODO add to settings
//     char* secret = nvs_gets(nvs_handle, "secret");

//     snprintf(url, strmax, "%s://%s:%d/join", http_prefix, host, http_port);
//     snprintf(post_data, strmax, "client=%s&secret=%s&%s", uids, secret, additional_join_post_data);

//     bool ret = http_request(HTTP_METHOD_POST, url, post_data, http_response_handler);

//     free(secret);
//     free(http_prefix);
//     free(host);
//     free(uids);

//     return ret;
// }

// typedef void (*additional_join_post_data_cb_t)(char* buff, size_t size); 
// typedef void (*app_main_presetup_func_t)(void);

// void app_main_start(
//     nvs_handle_t nvs_handle, 
//     additional_join_post_data_cb_t additional_join_post_data_cb, 
//     app_main_presetup_func_t presetup,
//     TaskFunction_t subtask
// ) {
//     presetup();

//     char* wifi_settings = app_wifi_start(nvs_handle);
    
//     TaskHandle_t xSubTaskHandle = NULL;
//     const size_t subtask_stack_size = 10000;
//     xTaskCreate(subtask, "subtask", subtask_stack_size, NULL, tskIDLE_PRIORITY, &xSubTaskHandle);

//     const size_t additional_join_post_data_size = 1000;
//     char* additional_join_post_data = (char*)calloc(additional_join_post_data_size, sizeof(char));
//     if (!additional_join_post_data) ERROR("Memory allocation error.");

//     while(1) {
//         additional_join_post_data_cb(additional_join_post_data, additional_join_post_data_size);
//         app_http_request_join(nvs_handle, additional_join_post_data, app_http_response_handler);
//         vTaskDelay(3000 / portTICK_RATE_MS);
//     }

//     free(additional_join_post_data);

//     if( xSubTaskHandle != NULL ) vTaskDelete( xSubTaskHandle );
    
//     free(wifi_settings);
// }


// /****************** MOTION MODE *****************/


// //CAMERA_MODEL_AI_THINKER PIN Map
// #define CAM_PIN_PWDN    (gpio_num_t)32 //power down is not used
// #define CAM_PIN_RESET   (gpio_num_t)-1 //software reset will be performed
// #define CAM_PIN_XCLK    (gpio_num_t)0
// #define CAM_PIN_SIOD    (gpio_num_t)26
// #define CAM_PIN_SIOC    (gpio_num_t)27

// #define CAM_PIN_D7      (gpio_num_t)35
// #define CAM_PIN_D6      (gpio_num_t)34
// #define CAM_PIN_D5      (gpio_num_t)39
// #define CAM_PIN_D4      (gpio_num_t)36
// #define CAM_PIN_D3      (gpio_num_t)21
// #define CAM_PIN_D2      (gpio_num_t)19
// #define CAM_PIN_D1      (gpio_num_t)18
// #define CAM_PIN_D0       (gpio_num_t)5
// #define CAM_PIN_VSYNC   (gpio_num_t)25
// #define CAM_PIN_HREF    (gpio_num_t)23
// #define CAM_PIN_PCLK    (gpio_num_t)22

// static camera_config_t motion_camera_config = {
//     .pin_pwdn  = CAM_PIN_PWDN,
//     .pin_reset = CAM_PIN_RESET,
//     .pin_xclk = CAM_PIN_XCLK,
//     .pin_sscb_sda = CAM_PIN_SIOD,
//     .pin_sscb_scl = CAM_PIN_SIOC,

//     .pin_d7 = CAM_PIN_D7,
//     .pin_d6 = CAM_PIN_D6,
//     .pin_d5 = CAM_PIN_D5,
//     .pin_d4 = CAM_PIN_D4,
//     .pin_d3 = CAM_PIN_D3,
//     .pin_d2 = CAM_PIN_D2,
//     .pin_d1 = CAM_PIN_D1,
//     .pin_d0 = CAM_PIN_D0,
//     .pin_vsync = CAM_PIN_VSYNC,
//     .pin_href = CAM_PIN_HREF,
//     .pin_pclk = CAM_PIN_PCLK,

//     //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
//     .xclk_freq_hz = 20000000,
//     .ledc_timer = LEDC_TIMER_0,
//     .ledc_channel = LEDC_CHANNEL_0,

//     //.pixel_format = PIXFORMAT_RGB888, //PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
//     //.frame_size = FRAMESIZE_QVGA, //FRAMESIZE_UXGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

//     .pixel_format = PIXFORMAT_GRAYSCALE, //PIXFORMAT_RGB888, //PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
//     .frame_size = FRAMESIZE_96X96, //FRAMESIZE_QVGA, //FRAMESIZE_UXGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

//     .jpeg_quality = 0, //0-63 lower number means higher quality
//     .fb_count = 1 //if more than one, i2s runs in continuous mode. Use only with JPEG
// };


// camera_fb_t * fb = NULL;


// struct watch_s {
//     int x;
//     int y;
//     int size;
//     int raster;
//     int threshold;
// };

// typedef struct watch_s watch_t;


// void app_main_motion_init() {
    
//     //power up the camera if PWDN pin is defined
//     if(CAM_PIN_PWDN != -1){
//         pinMode(CAM_PIN_PWDN, OUTPUT);
//         digitalWrite(CAM_PIN_PWDN, LOW);
//     }

//     //initialize the camera
//     ESP_ERROR_CHECK( esp_camera_init(&motion_camera_config) );
// }

// void app_main_motion_show_diff(size_t diff_sum) {
//     char spc[] = "]]]]]]]]]]]]]]]]]]]]]]]]]]]]][";
//     char* s = spc; 
//     for (int i=0; i<29; i++) {
//         if (i*10>diff_sum) s[i] = ' ';
//     }
//     PRINTF("%s (%u)", s, diff_sum);
// }

// //----

// void app_main_motion_additional_join_post_data(char* buff, size_t size) {
//     strncpy(buff, "type=motion", size);
// }

// void app_main_motion_setup() {
//     PRINT("CAMERA INIT...");
//     app_main_motion_init();
//     PRINT("CAMERA INIT...OK");
// }

// void app_main_motion_loop(void * pvParameters) {

//     PRINT("START app_main_motion_loop...");

//     watch_t watcher = {43, 43, 10, 5, 100};
//     static const int watcher_size_max = 40;
//     uint8_t prev_buf[40*40*4];

//     static const size_t buff_size = watcher_size_max*watcher_size_max*4;

//     size_t diff_sum_max = 0;


//     int counter = 0;
//     while(1) {
//         PRINTF("counter:%d", counter++);
//         vTaskDelay(1000 / portTICK_RATE_MS);


//         // fb = esp_camera_fb_get();            
//         // if (!fb) PRINT("Motion Camera capture failed");
        
//         // int xfrom = watcher.x - watcher.size;
//         // int xto = watcher.x + watcher.size;
//         // int yfrom = watcher.y - watcher.size;
//         // int yto = watcher.y + watcher.size;
//         // int i=0;
//         // size_t diff_sum = 0;
//         // for (int x=xfrom; x<xto; x+=watcher.raster) {
//         //     for (int y=yfrom; y<yto; y+=watcher.raster) {
//         //         int diff = fb->buf[x+y*fb->width] - prev_buf[i];
//         //         diff_sum += (diff > 0 ? diff : -diff);
//         //         prev_buf[i] = fb->buf[x+y*fb->width];
//         //         i++;
//         //         if (i>=buff_size) {
//         //             PRINT("buff size too large");
//         //             break;
//         //         }
//         //     }
//         // }

//         // if (diff_sum_max < diff_sum) diff_sum_max = diff_sum;
        
//         // app_main_motion_show_diff(diff_sum);
//         // if (diff_sum >= watcher.threshold) {
//         //     PRINT("******************************************************");
//         //     PRINT("*********************** [ALERT] **********************");
//         //     PRINT("******************************************************");
//         // }
        
//         // esp_camera_fb_return(fb);
//     }
// }

// /****************** MAIN *****************/


// extern "C" void app_main()
// {
//     PRINT("EHH?!");
//     app_main_motion_init();
//     PRINT("WHAT?!");

//     // stdio_init();

//     // nvs_handle_t nvs_handle;
//     // nvs_init(&nvs_handle);
//     // gpio_pins_init();

//     // int motion_button_state = !gpio_get_level(GPIO_NUM_12);
//     // int camera_button_state = !gpio_get_level(GPIO_NUM_13);

//     // if (motion_button_state) 
//     //     app_main_start(nvs_handle, app_main_motion_additional_join_post_data, app_main_motion_setup, app_main_motion_loop); //wifi_websocket_client_app_start(motion_sensor_setup, motion_sensor_loop, motion_sensor_data, motion_sensor_connected, motion_sensor_disconnected, motion_sensor_error);
//     // else if (camera_button_state) 
//     //     printf("CAMERA MODE...\n"); //wifi_websocket_client_app_start(camera_recorder_setup, camera_recorder_loop, camera_recorder_data, camera_recorder_connected, camera_recorder_disconnected, camera_recorder_error);
//     // else 
//     //     app_main_settings(nvs_handle);

//     // nvs_close(nvs_handle);
//     // stop("LEAVING");
// }



















// #include "esp_err.h"
// #include "esp_vfs_dev.h"
// #include "esp_wifi.h"
// #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
// #include "esp_log.h"
// #include "esp_event.h"
// #include "esp_tls.h"
// #include "esp_http_client.h"
// #include "driver/uart.h"
// #include "driver/gpio.h"
// #include "nvs_flash.h"
// #include "nvs.h"
// #include "sdkconfig.h"

// #include "esp_camera.h"
// #include "xclk.h"
// #include "sccb.h"
// #include "ov2640.h"
// #include "sensor.h"
// #include "yuv.h"
// #include "twi.h"
// #include "esp_jpg_decode.h"

// static const char *TAG = "espcam";
// #include "xclk.c"
// #include "ov2640.c"
// #include "sccb.c"
// #include "sensor.c"
// #include "yuv.c"
// #include "twi.c"
// #include "to_bmp.c"
// #include "esp_jpg_decode.c"
// #include "camera.c"


// /****************** COMMON *****************/

// // -------- IO ---------


// #define PRINT(msg) println(msg)
// #define ERROR(msg) errorln(__LINE__, msg)
// #define DEBUG(msg) debugln(__LINE__, msg)

// #define PRINTF(fmt, ...) printlnf(fmt, __VA_ARGS__)
// #define ERRORF(fmt, ...) errorlnf(__LINE__, fmt, __VA_ARGS__)
// #define DEBUGF(fmt, ...) debuglnf(__LINE__, fmt, __VA_ARGS__)

// // void println(const char* msg) {
// //     printf("%s\n", msg ? msg : "");
// // }

// void println(const char* msg) {
//     printf("%s\n", msg);
// }

// bool errorln(int line, const char* msg) {
//     printf("ERROR at line %d: %s\n", line, msg);
//     return false;
// }

// void debugln(int line, const char* msg) {
//     printf("DEBUG at line %d: %s", line, msg);
// }

// const size_t print_buffer_size = 1000;

// void printlnf(const char* fmt, ...) {
//     char buffer[print_buffer_size] = {0};
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buffer, sizeof(buffer), fmt, args);
//     va_end(args);

//     println(buffer);
// }

// bool errorlnf(int line, const char* fmt, ...) {
//     char buffer[print_buffer_size] = {0};
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buffer, sizeof(buffer), fmt, args);
//     va_end(args);

//     errorln(line, buffer);
//     esp_restart();

//     return false;
// }

// void debuglnf(int line, const char* fmt, ...) {
//     char buffer[print_buffer_size] = {0};
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buffer, sizeof(buffer), fmt, args);
//     va_end(args);

//     debugln(line, buffer);
// }

// void stop(const char* msg) {
//     PRINT(msg);
//     esp_restart();
// }

// char* reads(char* buff, size_t size) {
//     //char* buff = (char*)malloc(sizeof(char) * size);
//     //if (!buff) ERRORF("Memory allocation of %ld chars error.", size);
//     if (!fgets(buff, size, stdin)) ERROR("Read error.");
//     buff[strlen(buff)-1] = '\0';
//     return buff;
// }


// // -------- STRING ---------

// int get_str_pieces(const char* str, char delim) {
//     int i=0, pieces = 1;
//     while(str[i]) {
//         if(str[i] == delim) pieces++;
//         i++;
//     }
//     return pieces;
// }

// int get_str_piece_at(char* buff, size_t size, const char* str, char delim, int pos) {
//     int i=0, j=0, p = 0;
//     buff[j] = 0;
//     while(str && str[i]) {
//         if (p == pos) {     
//             if(str[i] == delim) break;
//             buff[j] = str[i];
//             if (size <= j) break;
//             j++;
//         }
//         if(str[i] == delim) p++;
//         i++;
//     }
//     buff[j] = 0;
//     return j;
// }

// /****************** STDIO *****************/

// void stdio_init(void)
// {
//     // Initialize VFS & UART so we can use std::cout/cin
//     if (setvbuf(stdin, NULL, _IONBF, 0)) ERROR("Initialize VFS & UART error.");
//     /* Install UART driver for interrupt-driven reads and writes */
//     ESP_ERROR_CHECK( uart_driver_install( (uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0) );
//     /* Tell VFS to use UART driver */
//     esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
//     esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
//     /* Move the caret to the beginning of the next line on '\n' */
//     esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
// }

// /****************** NVS *****************/

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

// uint16_t nvs_getu16(nvs_handle_t nvs_handle, const char* key, uint16_t default_value) {
//     uint16_t out_value = default_value;
//     esp_err_t err = nvs_get_u16(nvs_handle, key, &out_value);
//     if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) ESP_ERROR_CHECK(err);
//     return out_value;
// }

// char* nvs_gets(nvs_handle_t nvs_handle, const char* key) {
//     size_t stored_value_size = 100;
//     char* value_buff = (char*)malloc(sizeof(char) * stored_value_size);
//     if (!value_buff) ERROR("Memory allocation error.");
//     esp_err_t err = nvs_get_str(nvs_handle, key, NULL, &stored_value_size);
//     if (err == ESP_ERR_NVS_NOT_FOUND) {
//         value_buff[0] = '\0';
//         return value_buff;
//     } else if (err == ESP_OK) {
//         value_buff = (char*)realloc(value_buff, sizeof(char) * stored_value_size);
//         if (!value_buff) ERRORF("Memory of %ld chars error.", stored_value_size);
//         err = nvs_get_str(nvs_handle, key, value_buff, &stored_value_size);
//     }
//     ESP_ERROR_CHECK(err);
//     return value_buff;
// }

// void nvs_setu16(nvs_handle_t nvs_handle, const char* key, uint16_t value) {
//     esp_err_t err = nvs_set_u16(nvs_handle, key, value);
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


// // --------


// bool settings_validate_uids(char* value, bool halt) {
//     if (NULL == value || value[0] == '\0') return halt ? ERROR("Value is missing.") : false;
//     if (strlen(value) != 32) return halt ? ERROR("Incorrect UIDS length.") : false;
//     return true;
// }

// bool settings_validate_wifi_credentials(char* value, bool halt) {
//     if (NULL == value || value[0] == '\0')
//         return halt ? ERROR("WIFI credential settings are missing.") : false;
//     int num = get_str_pieces(value, ';');
//     for (int i=0; i<num; i++) {
//         size_t size = 100;
//         char buff[size];
//         get_str_piece_at(buff, size, value, ';', i);
//         if (get_str_pieces(buff, ':') < 2) 
//             return halt ? ERROR("WIFI credential settings is incorrect.") : false;
//     }
//     return true;
// }

// bool settings_validate_required(char* value, bool halt) {
//     if (NULL == value || value[0] == '\0') return halt ? ERROR("Value is missing.") : false;
//     return true;
// }

// bool settings_validate_numeric(char* value, bool halt) {
//     size_t len = strlen(value);
//     for (size_t i=0; i<len; i++) {
//         if (value[i] < '0' || value[i] > '9') return halt ? ERROR("Value is not numeric.") : false;
//     }
//     return true;
// }

// // ------

// void get_wifi_credential_from_settings_at(
//     char* wifi_settings, 
//     char* ssid_buff, size_t ssid_size, 
//     char* pwd_buff, size_t pwd_size, 
//     size_t at) 
// {
//     size_t wifi_credential_size = 100;
//     char wifi_credential_buff[wifi_credential_size];
//     get_str_piece_at(wifi_credential_buff, wifi_credential_size, wifi_settings, ';', at);

//     get_str_piece_at(ssid_buff, ssid_size, wifi_credential_buff, ':', 0);
//     get_str_piece_at(pwd_buff, pwd_size, wifi_credential_buff, ':', 1); 

// }

// bool get_wifi_pwd(char* wifi_settings, const char* ssid, char* pwd_buff, size_t pwd_size) {

//     size_t ssid_size = 32;
//     char ssid_buff[ssid_size];

//     size_t wifi_credentials_num = get_str_pieces(wifi_settings, ';');
//     for (int i=0; i<wifi_credentials_num; i++) {
//         get_wifi_credential_from_settings_at(wifi_settings, ssid_buff, ssid_size, pwd_buff, pwd_size, i);

//         if (!strcmp(ssid, ssid_buff)) return true;
//     }

//     pwd_size = 0;
//     pwd_buff[0] = '\0';

//     return false;
// }

// /****************** GPIO SETUP *****************/

// void gpio_pins_init() {
//     gpio_config_t io_conf = {
//         .pin_bit_mask = (1ULL<<GPIO_NUM_12) | (1ULL<<GPIO_NUM_13),
//         .mode = GPIO_MODE_INPUT,
//         .pull_up_en = GPIO_PULLUP_ENABLE,
//         .pull_down_en = GPIO_PULLDOWN_DISABLE,
//         .intr_type = GPIO_INTR_DISABLE,
//     };
//     ESP_ERROR_CHECK( gpio_config(&io_conf) );
// }

// /****************** SETTINGS MODE *****************/

// void app_main_settings(nvs_handle_t nvs_handle)
// {
//     PRINT("WAITING FOR CAMSYS SETTINGS...");

//     bool finish = false;

//     const size_t reads_size = 500;
//     char input[reads_size] = {0};
//     char value[reads_size] = {0};

//     while (!finish) {
//         input[0] = 0;
//         reads(input, reads_size);
        
//         if (!strcmp(input, "UID STRING:")) {

//             reads(value, reads_size);

//             if (!settings_validate_uids(value, false)) {
//                 PRINT("Invalid UIDS format.");
//             } else {
//                 nvs_sets(nvs_handle, "uids", value);
//                 PRINT("UIDS accepted.");
//             }

//         } else if (!strcmp(input, "WIFI CREDENTIALS:")) {

//             reads(value, reads_size);

//             if (!settings_validate_wifi_credentials(value, false)) {
//                 PRINT("Invalid WiFi credentials format.");
//             } else {
//                 nvs_sets(nvs_handle, "wifi", value);
//                 PRINT("Wifi credentials are accepted.");
//             }

//         } else if (!strcmp(input, "HOST ADDRESS OR IP:")) {

//             reads(value, reads_size);

//             if (!settings_validate_required(value, false)) {
//                 PRINT("Invalid host address format.");
//             } else {
//                 nvs_sets(nvs_handle, "host", value);
//                 PRINT("Host address is accepted.");
//             }

//         } else if (!strcmp(input, "SECRET:")) {

//             reads(value, reads_size);

//             if (!settings_validate_required(value, false)) {
//                 PRINT("Invalid secret format.");
//             } else {
//                 nvs_sets(nvs_handle, "secret", value);
//                 PRINT("Secret is accepted.");
//             }

//         } else if (!strcmp(input, "HTTP PREFIX:")) {

//             reads(value, reads_size);

//             if (!settings_validate_required(value, false)) {
//                 PRINT("Invalid http prefix format.");
//             } else {
//                 nvs_sets(nvs_handle, "http_prefix", value);
//                 PRINT("Http prefix is accepted.");
//             }

//         } else if (!strcmp(input, "HTTP PORT:")) {

//             reads(value, reads_size);

//             if (!settings_validate_required(value, false) || !settings_validate_numeric(value, false) ) {
//                 PRINT("Invalid http port format.");
//             } else {
//                 uint16_t value_u16 = atoi(value);
//                 nvs_setu16(nvs_handle, "http_prefix", value_u16);
//                 PRINT("Http port is accepted.");
//             }

//         } else if (!strcmp(input, "COMMIT")) {

//             finish = true;
//             nvs_store(nvs_handle);
//             PRINT("Settings are saved.");

//         } else {
//             PRINTF("Wrong settings argument: (%s)", input);
//         }

//     }



//     PRINT("FINISHED");
// }


// /****************** WIFI *****************/


// bool wifi_connected = false;
// static ip4_addr_t wifi_ip_addr;

// void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
//     //PRINTF("WIFI: ***EVENT (%ld)", event_id);

//     if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//         //PRINT("WIFI: GOT IP");
//         wifi_connected = true;
//         ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
//         memcpy(&wifi_ip_addr, &event->ip_info.ip, sizeof(wifi_ip_addr));
//         ESP_ERROR_CHECK(esp_wifi_scan_stop());
//     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//         //PRINT("WIFI: DISCONNECTED");
//         wifi_connected = false;
//         int wifi_reconnect_attempts = 30;
//         while (!wifi_connected && wifi_reconnect_attempts) {
//             wifi_reconnect_attempts--;
//             PRINTF("WIFI: RECONNECT ATTEMPTS: %d", wifi_reconnect_attempts);
//             esp_wifi_connect();
//             vTaskDelay(3000 / portTICK_RATE_MS);
//         }
//         if (!wifi_connected) {
//             ERROR("WIFI DISCONNECTED, RESTART...");
//             esp_restart();
//         }
//     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
//         //PRINT("WIFI: CONNECTED");
//     }
// }

// bool wifi_connect(const char* ssid, const char* pwd) {

//     wifi_config_t wifi_config;
//     //wifi_config.sta.ssid[0] = '\0';
//     //wifi_config.sta.password[0] = '\0';
//     wifi_config.sta.bssid_set = 0;
//     wifi_config.sta.pmf_cfg.required = false;

//     strncpy((char*)wifi_config.sta.ssid, ssid, 32);
//     strncpy((char*)wifi_config.sta.password, pwd, 64);
    
//     ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
//     ESP_ERROR_CHECK(esp_wifi_connect());
    
//     //vTaskDelay(3000 / portTICK_RATE_MS);
//     while (!wifi_connected) vTaskDelay(300 / portTICK_RATE_MS); //return false;

//     PRINTF("Connected to %s", ssid);
//     PRINTF("IPv4 address: " IPSTR, IP2STR(&wifi_ip_addr));
//     return true;
// }

// bool wifi_connect_attempt(char* wifi_settings, char* ssid) {
//     size_t pwd_size = 64;
//     char pwd_buff[pwd_size];
//     if (get_wifi_pwd(wifi_settings, ssid, pwd_buff, pwd_size)) {
//         // TODO connect...
//         //PRINTF("Attempt to connect to '%s'...", ssid);
//         return wifi_connect(ssid, pwd_buff);
//     }
//     return false;
// }

// bool wifi_scan_connect(char* wifi_settings)
// {

//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
//     assert(sta_netif);

//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

//     ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL) );
//     ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL) );

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
//     ESP_LOGI("camsys", "Total APs scanned = %u", ap_count);
//     for (int i = 0; (i < default_scan_list_size) && (i < ap_count); i++) {
//         ESP_LOGI("camsys", "SSID \t\t%s", ap_info[i].ssid);
//         ESP_LOGI("camsys", "RSSI \t\t%d", ap_info[i].rssi);        
//         ESP_LOGI("camsys", "Channel \t\t%d\n", ap_info[i].primary);

//         if (wifi_connect_attempt(wifi_settings, (char*)ap_info[i].ssid)) {
//             return true;
//         }
//     }

//     return false;
// }

// char* app_wifi_start(nvs_handle_t nvs_handle) {
//     char* wifi_settings = nvs_gets(nvs_handle, "wifi");
//     settings_validate_wifi_credentials(wifi_settings, true);
//     if (!wifi_scan_connect(wifi_settings)) ERROR("WIFI connect error");
//     return wifi_settings;
// }
// /****************** HTTP CLIENT *****************/

// typedef void (*http_response_handler_t)(char* buff, size_t size);

// http_response_handler_t _http_response_handler = NULL;

// esp_err_t _http_event_handler(esp_http_client_event_t *evt)
// {
//     static char *output_buffer;  // Buffer to store response of http request from event handler
//     static int output_len;       // Stores number of bytes read
//     switch(evt->event_id) {
//         case HTTP_EVENT_ERROR:
//             // ESP_LOGD("camsys", "HTTP_EVENT_ERROR");
//             // PRINT("HTTP ERROR");
//             break;
//         case HTTP_EVENT_ON_CONNECTED:
//             // ESP_LOGD("camsys", "HTTP_EVENT_ON_CONNECTED");
//             // PRINT("HTTP ON CONNECTED");
//             break;
//         case HTTP_EVENT_HEADER_SENT:
//             // ESP_LOGD("camsys", "HTTP_EVENT_HEADER_SENT");
//             // PRINT("HTTP HEADER SENT");
//             break;
//         case HTTP_EVENT_ON_HEADER:
//             // ESP_LOGD("camsys", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
//             // PRINT("HTTP ON HEADER");
//             break;
//         case HTTP_EVENT_ON_DATA:
//             // ESP_LOGD("camsys", "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
//             // PRINT("HTTP ON DATA");
//             /*
//              *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
//              *  However, event handler can also be used in case chunked encoding is used.
//              */
//             if (!esp_http_client_is_chunked_response(evt->client)) {
//                 // If user_data buffer is configured, copy the response into the buffer
//                 if (evt->user_data) {
//                     memcpy((char*)evt->user_data + output_len, evt->data, evt->data_len);
//                 } else {
//                     if (output_buffer == NULL) {
//                         output_buffer = (char *) calloc(esp_http_client_get_content_length(evt->client) + 1, sizeof(char));
//                         output_len = 0;
//                         if (output_buffer == NULL) {
//                             ESP_LOGE("camsys", "Failed to allocate memory for output buffer");
//                             return ESP_FAIL;
//                         }
//                     }
//                     memcpy(output_buffer + output_len, evt->data, evt->data_len);
//                 }
//                 output_len += evt->data_len;
//             }

//             break;
//         case HTTP_EVENT_ON_FINISH:
//             // ESP_LOGD("camsys", "HTTP_EVENT_ON_FINISH");
//             // PRINT("HTTP ON FINISH");
//             if (output_buffer != NULL) {
//                 // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
//                 // ESP_LOG_BUFFER_HEX("camsys", output_buffer, output_len);
//                 if (_http_response_handler) _http_response_handler(output_buffer, output_len);
//                 free(output_buffer);
//                 output_buffer = NULL;
//             }
//             output_len = 0;
//             break;
//         case HTTP_EVENT_DISCONNECTED:
//             // ESP_LOGI("camsys", "HTTP_EVENT_DISCONNECTED");
//             // PRINT("HTTP DISCONNECTED");
//             int mbedtls_err = 0;
//             esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
//             if (err != 0) {
//                 if (output_buffer != NULL) {
//                     free(output_buffer);
//                     output_buffer = NULL;
//                 }
//                 output_len = 0;
//                 ESP_LOGI("camsys", "Last esp error code: 0x%x", err);
//                 ESP_LOGI("camsys", "Last mbedtls failure: 0x%x", mbedtls_err);
//             }
//             break;
//     }
//     return ESP_OK;
// }

// // TODO: may it should be in the esp-idf lib folder as a macro just like `wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();`, (see at esp_wifi.h) later on mey I will create a pull request?

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


// bool http_request(
//     esp_http_client_method_t method, const char* url, const char* post_data,
//     http_response_handler_t response_handler
// ) {
//     esp_http_client_config_t config = HTTP_CLIENT_CONFIG_DEFAULT(url);
//     config.method = method;
//     config.event_handler = _http_event_handler;

//     esp_http_client_handle_t client = esp_http_client_init(&config);
//     esp_http_client_set_method(client, method);
//     esp_http_client_set_post_field(client, post_data, strlen(post_data));
    

//     // PRINT("HTTP PERFOM...");
//     _http_response_handler = response_handler;
//     esp_err_t err = esp_http_client_perform(client);
//     // PRINT("HTTP PERFORMED.");
//     if (err == ESP_OK) {
//         // ESP_LOGI("camsys", "HTTP POST Status = %d, content_length = %d",
//         //         esp_http_client_get_status_code(client),
//         //         esp_http_client_get_content_length(client));
//     } else {
//         ESP_LOGE("camsys", "HTTP POST request failed: %s", esp_err_to_name(err));
//     }

//     esp_http_client_cleanup(client);
//     return err == ESP_OK;
// }

// void app_http_response_handler(char* buff, size_t size) {
//     PRINTF("RESPONSE STRING LENGTH: %d, BUFF_SIZE: %d", strlen(buff), size);
// }


// /****************** APP TASKS STARTER *****************/

// bool app_http_request_join(nvs_handle_t nvs_handle, const char* additional_join_post_data, http_response_handler_t http_response_handler) {
//     const size_t strmax = 255;
//     char url[strmax];
//     char post_data[strmax];

//     char* uids = nvs_gets(nvs_handle, "uids"); // TODO add to settings
//     char* host = nvs_gets(nvs_handle, "host");
//     char* http_prefix = nvs_gets(nvs_handle, "http_prefix"); // TODO add to settings
//     uint16_t http_port = nvs_getu16(nvs_handle, "http_port", 3000); // TODO add to settings
//     char* secret = nvs_gets(nvs_handle, "secret");

//     snprintf(url, strmax, "%s://%s:%d/join", http_prefix, host, http_port);
//     snprintf(post_data, strmax, "client=%s&secret=%s&%s", uids, secret, additional_join_post_data);

//     bool ret = http_request(HTTP_METHOD_POST, url, post_data, http_response_handler);

//     free(secret);
//     free(http_prefix);
//     free(host);
//     free(uids);

//     return ret;
// }

// typedef void (*additional_join_post_data_cb_t)(char* buff, size_t size); 
// typedef void (*app_main_presetup_func_t)(void);

// void app_main_start(
//     nvs_handle_t nvs_handle, 
//     additional_join_post_data_cb_t additional_join_post_data_cb, 
//     app_main_presetup_func_t presetup,
//     TaskFunction_t subtask
// ) {
//     presetup();

//     char* wifi_settings = app_wifi_start(nvs_handle);
    
//     TaskHandle_t xSubTaskHandle = NULL;
//     const size_t subtask_stack_size = 10000;
//     xTaskCreate(subtask, "subtask", subtask_stack_size, NULL, tskIDLE_PRIORITY, &xSubTaskHandle);

//     const size_t additional_join_post_data_size = 1000;
//     char* additional_join_post_data = (char*)calloc(additional_join_post_data_size, sizeof(char));
//     if (!additional_join_post_data) ERROR("Memory allocation error.");

//     while(1) {
//         additional_join_post_data_cb(additional_join_post_data, additional_join_post_data_size);
//         app_http_request_join(nvs_handle, additional_join_post_data, app_http_response_handler);
//         vTaskDelay(3000 / portTICK_RATE_MS);
//     }

//     free(additional_join_post_data);

//     if( xSubTaskHandle != NULL ) vTaskDelete( xSubTaskHandle );
    
//     free(wifi_settings);
// }


// /****************** MOTION MODE *****************/


// //CAMERA_MODEL_AI_THINKER PIN Map
// #define CAM_PIN_PWDN    (gpio_num_t)32 //power down is not used
// #define CAM_PIN_RESET   (gpio_num_t)-1 //software reset will be performed
// #define CAM_PIN_XCLK    (gpio_num_t)0
// #define CAM_PIN_SIOD    (gpio_num_t)26
// #define CAM_PIN_SIOC    (gpio_num_t)27

// #define CAM_PIN_D7      (gpio_num_t)35
// #define CAM_PIN_D6      (gpio_num_t)34
// #define CAM_PIN_D5      (gpio_num_t)39
// #define CAM_PIN_D4      (gpio_num_t)36
// #define CAM_PIN_D3      (gpio_num_t)21
// #define CAM_PIN_D2      (gpio_num_t)19
// #define CAM_PIN_D1      (gpio_num_t)18
// #define CAM_PIN_D0       (gpio_num_t)5
// #define CAM_PIN_VSYNC   (gpio_num_t)25
// #define CAM_PIN_HREF    (gpio_num_t)23
// #define CAM_PIN_PCLK    (gpio_num_t)22

// static camera_config_t motion_camera_config = {
//     .pin_pwdn  = CAM_PIN_PWDN,
//     .pin_reset = CAM_PIN_RESET,
//     .pin_xclk = CAM_PIN_XCLK,
//     .pin_sscb_sda = CAM_PIN_SIOD,
//     .pin_sscb_scl = CAM_PIN_SIOC,

//     .pin_d7 = CAM_PIN_D7,
//     .pin_d6 = CAM_PIN_D6,
//     .pin_d5 = CAM_PIN_D5,
//     .pin_d4 = CAM_PIN_D4,
//     .pin_d3 = CAM_PIN_D3,
//     .pin_d2 = CAM_PIN_D2,
//     .pin_d1 = CAM_PIN_D1,
//     .pin_d0 = CAM_PIN_D0,
//     .pin_vsync = CAM_PIN_VSYNC,
//     .pin_href = CAM_PIN_HREF,
//     .pin_pclk = CAM_PIN_PCLK,

//     //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
//     .xclk_freq_hz = 20000000,
//     .ledc_timer = LEDC_TIMER_0,
//     .ledc_channel = LEDC_CHANNEL_0,

//     //.pixel_format = PIXFORMAT_RGB888, //PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
//     //.frame_size = FRAMESIZE_QVGA, //FRAMESIZE_UXGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

//     .pixel_format = PIXFORMAT_GRAYSCALE, //PIXFORMAT_RGB888, //PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
//     .frame_size = FRAMESIZE_96X96, //FRAMESIZE_QVGA, //FRAMESIZE_UXGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

//     .jpeg_quality = 0, //0-63 lower number means higher quality
//     .fb_count = 1 //if more than one, i2s runs in continuous mode. Use only with JPEG
// };


// camera_fb_t * fb = NULL;


// struct watch_s {
//     int x;
//     int y;
//     int size;
//     int raster;
//     int threshold;
// };

// typedef struct watch_s watch_t;


// void app_main_motion_init() {
    
//     //power up the camera if PWDN pin is defined
//     if(CAM_PIN_PWDN != -1){
//         pinMode(CAM_PIN_PWDN, OUTPUT);
//         digitalWrite(CAM_PIN_PWDN, LOW);
//     }

//     //initialize the camera
//     ESP_ERROR_CHECK( esp_camera_init(&motion_camera_config) );
// }

// void app_main_motion_show_diff(size_t diff_sum) {
//     char spc[] = "]]]]]]]]]]]]]]]]]]]]]]]]]]]]][";
//     char* s = spc; 
//     for (int i=0; i<29; i++) {
//         if (i*10>diff_sum) s[i] = ' ';
//     }
//     PRINTF("%s (%u)", s, diff_sum);
// }

// //----

// void app_main_motion_additional_join_post_data(char* buff, size_t size) {
//     strncpy(buff, "type=motion", size);
// }

// void app_main_motion_setup() {
//     PRINT("CAMERA INIT...");
//     app_main_motion_init();
//     PRINT("CAMERA INIT...OK");
// }

// void app_main_motion_loop(void * pvParameters) {

//     PRINT("START app_main_motion_loop...");

//     watch_t watcher = {43, 43, 10, 5, 100};
//     static const int watcher_size_max = 40;
//     uint8_t prev_buf[40*40*4];

//     static const size_t buff_size = watcher_size_max*watcher_size_max*4;

//     size_t diff_sum_max = 0;


//     int counter = 0;
//     while(1) {
//         PRINTF("counter:%d", counter++);
//         vTaskDelay(1000 / portTICK_RATE_MS);


//         // fb = esp_camera_fb_get();            
//         // if (!fb) PRINT("Motion Camera capture failed");
        
//         // int xfrom = watcher.x - watcher.size;
//         // int xto = watcher.x + watcher.size;
//         // int yfrom = watcher.y - watcher.size;
//         // int yto = watcher.y + watcher.size;
//         // int i=0;
//         // size_t diff_sum = 0;
//         // for (int x=xfrom; x<xto; x+=watcher.raster) {
//         //     for (int y=yfrom; y<yto; y+=watcher.raster) {
//         //         int diff = fb->buf[x+y*fb->width] - prev_buf[i];
//         //         diff_sum += (diff > 0 ? diff : -diff);
//         //         prev_buf[i] = fb->buf[x+y*fb->width];
//         //         i++;
//         //         if (i>=buff_size) {
//         //             PRINT("buff size too large");
//         //             break;
//         //         }
//         //     }
//         // }

//         // if (diff_sum_max < diff_sum) diff_sum_max = diff_sum;
        
//         // app_main_motion_show_diff(diff_sum);
//         // if (diff_sum >= watcher.threshold) {
//         //     PRINT("******************************************************");
//         //     PRINT("*********************** [ALERT] **********************");
//         //     PRINT("******************************************************");
//         // }
        
//         // esp_camera_fb_return(fb);
//     }
// }

// /****************** MAIN *****************/


// extern "C" void app_main()
// {
//     PRINT("EHH?!");
//     app_main_motion_init();
//     PRINT("WHAT?!");

//     // stdio_init();

//     // nvs_handle_t nvs_handle;
//     // nvs_init(&nvs_handle);
//     // gpio_pins_init();

//     // int motion_button_state = !gpio_get_level(GPIO_NUM_12);
//     // int camera_button_state = !gpio_get_level(GPIO_NUM_13);

//     // if (motion_button_state) 
//     //     app_main_start(nvs_handle, app_main_motion_additional_join_post_data, app_main_motion_setup, app_main_motion_loop); //wifi_websocket_client_app_start(motion_sensor_setup, motion_sensor_loop, motion_sensor_data, motion_sensor_connected, motion_sensor_disconnected, motion_sensor_error);
//     // else if (camera_button_state) 
//     //     printf("CAMERA MODE...\n"); //wifi_websocket_client_app_start(camera_recorder_setup, camera_recorder_loop, camera_recorder_data, camera_recorder_connected, camera_recorder_disconnected, camera_recorder_error);
//     // else 
//     //     app_main_settings(nvs_handle);

//     // nvs_close(nvs_handle);
//     // stop("LEAVING");
// }




















//#include <stdio.h>
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "driver/gpio.h"
//#include "sdkconfig.h"


#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_eth.h"

#include "esp_http_server.h"

#include "protocol_examples_common.h"


#include "freertos/task.h"

static const char *TAG = "motion";

#include "ov2640.c"
#include "esp_camera.h"
#include "camera.c"
#include "xclk.c"
#include "sccb.c"
#include "sensor.c"
#include "yuv.c"
#include "twi.c"


#include "to_bmp.c"
#include "esp_jpg_decode.c"



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

static camera_config_t motion_camera_config = {
    .pin_pwdn  = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    //.pixel_format = PIXFORMAT_RGB888, //PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    //.frame_size = FRAMESIZE_QVGA, //FRAMESIZE_UXGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

    .pixel_format = PIXFORMAT_GRAYSCALE, //PIXFORMAT_RGB888, //PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_96X96, //FRAMESIZE_QVGA, //FRAMESIZE_UXGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 0, //0-63 lower number means higher quality
    .fb_count = 1 //if more than one, i2s runs in continuous mode. Use only with JPEG
};


void app_error(esp_err_t err) {
    ESP_LOGE(TAG, "ERROR: %u", err);
    while(1);
}

void app_warn(esp_err_t err) {
    ESP_LOGE(TAG, "WARN: %u", err);
}

void show_diff(size_t diff_sum) {
    char spc[] = "]]]]]]]]]]]]]]]]]]]]]]]]]]]]][";
    char* s = spc; 
    for (int i=0; i<29; i++) {
        if (i*10>diff_sum) s[i] = ' ';
    }
    ESP_LOGI(TAG, "%s (%u)", s, diff_sum);
}

/*****************************/

camera_fb_t * fb = NULL;


struct watch_s {
    int x;
    int y;
    int size;
    int raster;
    int threshold;
};

typedef struct watch_s watch_t;

watch_t watcher = {43, 43, 10, 5, 100};
static const int watcher_size_max = 40;
uint8_t prev_buf[40*40*4];

void motion_init() {
    
    ESP_LOGI(TAG, "******INIT*******");
        
    //initialize the camera
    esp_err_t err = esp_camera_init(&motion_camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Motion Camera Init Failed");
        app_error(err);
    }   
    ESP_LOGI(TAG, "******INIT OK*******");
}

size_t diff_sum_max = 0;

void motion_detect() {
//    size_t raster = 3000;
    static const size_t buff_size = watcher_size_max*watcher_size_max*4;
    
    motion_init();
    
    while(1) {
        //vTaskDelay(1000 / portTICK_PERIOD_MS); 
        
        //int64_t fr_start = esp_timer_get_time();
        
        //ESP_LOGI(TAG, "******CAPTURE*******");
        fb = esp_camera_fb_get();            
        if (!fb) {
            ESP_LOGE(TAG, "Motion Camera capture failed");
            app_warn(ESP_FAIL);
        }
        //ESP_LOGI(TAG, "FRAME BUFFER: %u %u %u", fb->len, fb->width, fb->height);
        
        
//        //ESP_LOGI(TAG, "******DIFF*******");
//        size_t diff_sum = 0;
//        for (size_t i=0; i<fb->len/raster; i++) {
//            int diff = fb->buf[i*raster] - prev_buf[i];
//            //ESP_LOGI(TAG, "DIFF: %u, (%u %u)", diff, fb->buf[i], prev_buf[i]);
//            diff_sum += (diff > 0 ? diff : -diff);
//            prev_buf[i] = fb->buf[i*raster];       
//        }
//        //ESP_LOGI(TAG, "DIFF SUM: %u", diff_sum);
//        show_diff(diff_sum);
        
        int xfrom = watcher.x - watcher.size;
        int xto = watcher.x + watcher.size;
        int yfrom = watcher.y - watcher.size;
        int yto = watcher.y + watcher.size;
        int i=0;
        size_t diff_sum = 0;
        for (int x=xfrom; x<xto; x+=watcher.raster) {
            for (int y=yfrom; y<yto; y+=watcher.raster) {
                int diff = fb->buf[x+y*fb->width] - prev_buf[i];
                diff_sum += (diff > 0 ? diff : -diff);
                prev_buf[i] = fb->buf[x+y*fb->width];
                i++;
                if (i>=buff_size) {
                    ESP_LOGE(TAG, "buff size too large");
                    break;
                }
            }
        }

//        fb->buf[0] = diff_sum8_max;
//        
//        // TODO: mac address back
//        fb->buf[1] = 0x12;
//        fb->buf[2] = 0x34;
//        fb->buf[3] = 0x56;
//        fb->buf[4] = 0x78;
//        fb->buf[5] = 0x9a;
//        fb->buf[6] = 0xbc;
        
        if (diff_sum_max < diff_sum) diff_sum_max = diff_sum;
        
        show_diff(diff_sum);
        if (diff_sum >= watcher.threshold) {
            ESP_LOGI(TAG, "******************************************************");
            ESP_LOGI(TAG, "*********************** [ALERT] **********************");
            ESP_LOGI(TAG, "******************************************************");
        }
        
        esp_camera_fb_return(fb);
    
        //int64_t fr_end = esp_timer_get_time();
        //ESP_LOGI(TAG, "Loop: %ums", (uint32_t)((fr_end - fr_start)/1000));
    }
}


/*****************HTTP ******************/
esp_err_t bmp_httpd_handler(httpd_req_t* req) {
    //camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    int64_t fr_start = esp_timer_get_time();
	
    //initialize the camera
    if (!fb) {
        //motion_init();

        fb = esp_camera_fb_get();
    }
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "FRAME BUFFER: %u %u %u", fb->len, fb->width, fb->height);
	
    uint8_t * buf = NULL;
    size_t buf_len = 0;
    bool converted = frame2bmp(fb, &buf, &buf_len);
    esp_camera_fb_return(fb);
    if(!converted){
        ESP_LOGE(TAG, "BMP conversion failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // TODO: convert to jpeg to get faster net response
    res = httpd_resp_set_type(req, "image/x-windows-bmp")
       || httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.bmp")
       || httpd_resp_send(req, (const char *)buf, buf_len);
    free(buf);
    int64_t fr_end = esp_timer_get_time();
    ESP_LOGI(TAG, "BMP: %uKB %ums", (uint32_t)(buf_len/1024), (uint32_t)((fr_end - fr_start)/1000));
    return res;
}

static const httpd_uri_t motion = {
    .uri       = "/motion",
    .method    = HTTP_GET,
    .handler   = bmp_httpd_handler, // TODO: JPEG FASTER!! use/convert jpeg at motion monitor to mark fixed pixels on screen to get faster net speeed
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

esp_err_t watch_httpd_handler(httpd_req_t* req) {
    char*  buf;
    size_t buf_len;
//
//    /* Get header value string length and allocate memory for length + 1,
//     * extra byte for null termination */
//    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
//    if (buf_len > 1) {
//        buf = malloc(buf_len);
//        /* Copy null terminated value string into buffer */
//        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
//            ESP_LOGI(TAG, "Found header => Host: %s", buf);
//        }
//        free(buf);
//    }
//
//    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
//    if (buf_len > 1) {
//        buf = malloc(buf_len);
//        if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK) {
//            ESP_LOGI(TAG, "Found header => Test-Header-2: %s", buf);
//        }
//        free(buf);
//    }
//
//    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
//    if (buf_len > 1) {
//        buf = malloc(buf_len);
//        if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK) {
//            ESP_LOGI(TAG, "Found header => Test-Header-1: %s", buf);
//        }
//        free(buf);
//    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        int     x=watcher.x, 
                y=watcher.y, 
                size=watcher.size, 
                raster=watcher.raster,
                threshold = watcher.threshold;
        if (fb) {
            x=fb->len/2;
            y=fb->len/2;
            
            buf = malloc(buf_len);
            if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query => %s", buf);
                char param[32];
                /* Get value of expected key from query string */
                if (httpd_query_key_value(buf, "x", param, sizeof(param)) == ESP_OK) {
                    ESP_LOGI(TAG, "Found URL query parameter => x=%s", param);
                    x = atoi(param);
                }
                if (httpd_query_key_value(buf, "y", param, sizeof(param)) == ESP_OK) {
                    ESP_LOGI(TAG, "Found URL query parameter => y=%s", param);
                    y = atoi(param);
                }
                if (httpd_query_key_value(buf, "size", param, sizeof(param)) == ESP_OK) {
                    ESP_LOGI(TAG, "Found URL query parameter => size=%s", param);
                    size = atoi(param);
                }
                if (httpd_query_key_value(buf, "raster", param, sizeof(param)) == ESP_OK) {
                    ESP_LOGI(TAG, "Found URL query parameter => raster=%s", param);
                    raster = atoi(param);
                }
                if (httpd_query_key_value(buf, "threshold", param, sizeof(param)) == ESP_OK) {
                    ESP_LOGI(TAG, "Found URL query parameter => threshold=%s", param);
                    threshold = atoi(param);
                }
            }
            
            if (size>watcher_size_max) size = watcher_size_max;
            if (raster>size) raster = size;
            if (x<size) x = size;
            if (y<size) y = size;
            if (x>fb->width-size) x = fb->width-size;
            if (y>fb->height-size) y = fb->height-size;
            free(buf);
        }
        watcher.x = x;
        watcher.y = y;
        watcher.size = size;
        watcher.raster = raster;
        watcher.threshold = threshold;
    }

//    /* Set some custom headers */
//    httpd_resp_set_hdr(req, "Custom-Header-1", "Custom-Value-1");
//    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");
//
//    /* Send response with custom headers and body set as the
//     * string passed in user context*/
//    const char* resp_str = (const char*) req->user_ctx;
//    httpd_resp_send(req, resp_str, strlen(resp_str));
//
//    /* After sending the HTTP response the old HTTP request
//     * headers are lost. Check if HTTP request headers can be read now. */
//    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
//        ESP_LOGI(TAG, "Request headers lost");
//    }
    return ESP_OK;
}

static const httpd_uri_t watch = {
    .uri       = "/watch",
    .method    = HTTP_GET,
    .handler   = watch_httpd_handler, // TODO: JPEG FASTER!! use/convert jpeg at motion monitor to mark fixed pixels on screen to get faster net speeed
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};

esp_err_t sensor_httpd_handler(httpd_req_t* req) {
//    //camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
//    int64_t fr_start = esp_timer_get_time();
//	
    //initialize the camera
    if (!fb) {
        //motion_init();

        fb = esp_camera_fb_get();
    }
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "DIFF SUM: %u", diff_sum_max);

    size_t w = fb->width;
    size_t h = fb->height;
    if (diff_sum_max > fb->width * fb->height) diff_sum_max = fb->width * fb->height;
    fb->width = diff_sum_max;
    fb->height = 1;
    diff_sum_max = 0;
    
    uint8_t * buf = NULL;
    size_t buf_len = 0;
    bool converted = frame2bmp(fb, &buf, &buf_len);
    esp_camera_fb_return(fb);
    
    fb->width = w;
    fb->height = h;
    
    if(!converted){
        ESP_LOGE(TAG, "FAKE BMP conversion failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // TODO: convert to jpeg to get faster net response
    res = httpd_resp_set_type(req, "image/x-windows-bmp")
       || httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=sensor.bmp")
       || httpd_resp_send(req, (const char *)buf, buf_len);
    free(buf);
//    int64_t fr_end = esp_timer_get_time();
//    ESP_LOGI(TAG, "BMP: %uKB %ums", (uint32_t)(buf_len/1024), (uint32_t)((fr_end - fr_start)/1000));
    return res;
}

static const httpd_uri_t sensor = {
    .uri       = "/sensor",
    .method    = HTTP_GET,
    .handler   = sensor_httpd_handler, // TODO: JPEG FASTER!! use/convert jpeg at motion monitor to mark fixed pixels on screen to get faster net speeed
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};


static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
//        httpd_register_uri_handler(server, &hello);
//        httpd_register_uri_handler(server, &echo);
        httpd_register_uri_handler(server, &motion);
        httpd_register_uri_handler(server, &watch);
        httpd_register_uri_handler(server, &sensor);
//        httpd_register_uri_handler(server, &ctrl);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void connect_handler(void* arg, esp_event_base_t event_base, 
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}


static void disconnect_handler(void* arg, esp_event_base_t event_base, 
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

void esp_server() {
    static httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
     * and re-start it upon connection.
     */
#ifdef CONFIG_EXAMPLE_CONNECT_WIFI
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_EXAMPLE_CONNECT_WIFI
#ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_EXAMPLE_CONNECT_ETHERNET

    /* Start the server for the first time */
    server = start_webserver();
}

void app_main()
{
    esp_server();
    motion_detect();
}
