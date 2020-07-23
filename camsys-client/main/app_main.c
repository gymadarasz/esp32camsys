#include <stdio.h>
#include "sdkconfig.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "lib/myesp/myesp.h"

static const char *TAG = "app";

/*

// ---------------------------------------------------------------
// DELAY
// ---------------------------------------------------------------

// ---------------------------------------------------------------
// ERROR
// ---------------------------------------------------------------

// ---------------------------------------------------------------
// SERIAL READ
// ---------------------------------------------------------------

    ESP_ERROR_CHECK( serial_install(UART_NUM_0, ESP_LINE_ENDINGS_CR, ESP_LINE_ENDINGS_CRLF) );

    ESP_LOGI(TAG, "Give me a string:\n");
    char* str = serial_readln();
    ESP_LOGI(TAG, "you wrote: '%s'\n", str ? str : "(read error)");
    free(str);

    ESP_ERROR_CHECK( serial_uninstall(UART_NUM_0) );

// ---------------------------------------------------------------
// SDCARD READ
// ---------------------------------------------------------------

    const char str[] = "Hello";
    ESP_ERROR_CHECK( sdcard_init(esp_vfs_fat_sdspi_mount, false, 5, 16 * 1024) );

    const char* fname = SDCARD_MOUNT_POINT"/test.txt";
    ESP_LOGI(TAG, "Saving it to file '%s'\n", fname);
    FILE* f = fopen(fname, "w");
    if (!f) ERROR("File open error.");
    if (fprintf(f, "Your text is '%s'!\n", str) < 0) ERRORF("File write error: %d", ferror(f));;
    if (fclose(f)) ERROR("File close error.");

    ESP_ERROR_CHECK( sdcard_close(esp_vfs_fat_sdcard_unmount) );
    
// ---------------------------------------------------------------
// STRING EXTRAS
// ---------------------------------------------------------------

// ---------------------------------------------------------------
// WIFI CREDENTIALS
// ---------------------------------------------------------------

    wifi_creds_t creds;
    wifi_creds_clear(creds);
    
    WIFI_CREDS_SET(creds, 0, "apuc", "mad12345a");
    WIFI_CREDS_SET(creds, 1, "foo", "bar");
    WIFI_CREDS_SET(creds, 2, "abc", "asd");
    // ..
    WIFI_CREDS_SET(creds, 6, "eee7", "");
    // ..
    WIFI_CREDS_SET(creds, 8, "eee6", "rrrr6");
    
    const char* found_ssid = "abc";
    
    for (int i=0; i<WIFI_CREDENTIALS; i++) {
        if (WIFI_CREDS_GET_SSID(creds, i)[0] && wifi_creds_get_pswd(creds, WIFI_CREDS_GET_SSID(creds, i) )) {
            ESP_LOGI(TAG, "could connect to: [%d] -> '%s', '%s'\n", i, WIFI_CREDS_GET_SSID(creds, i), WIFI_CREDS_GET_PSWD(creds, i));
            if (!strcmp(found_ssid, WIFI_CREDS_GET_SSID(creds, i))) {
                ESP_LOGI(TAG, "^^FOUND: attempt to connect...\n");
            }
        }
    }

    // ---------------------------------------------------------------

    wifi_creds_t creds;
    WIFI_CREDS_SET(creds, 0, "apucika", "mad12345");
    WIFI_CREDS_SET(creds, 1, "apucika_EXT", "mad12345");

    for (int i=0; i<WIFI_CREDENTIALS; i++) {
        if (WIFI_CREDS_GET_SSID(creds, i)[0])
            ESP_LOGI(TAG, "cred(%d):'%s':'%s' => '%s'\n", i, 
                WIFI_CREDS_GET_SSID(creds, i), 
                WIFI_CREDS_GET_PSWD(creds, i), 
                wifi_creds_get_pswd(creds,WIFI_CREDS_GET_SSID(creds, i))
            );
    }



*/

// ---------------------------------------------------------------
// WIFI CREDENTIALS
// ---------------------------------------------------------------

esp_err_t wifi_creds_save(wifi_creds_t creds, const char* namespace) {
    // Open
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace && namespace[0] ? namespace : MYESP_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    // Write
    err = nvs_set_blob(handle, "wifi_creds", creds, WIFI_CREDS_SIZE);
    if (err == ESP_OK) err = nvs_commit(handle);

    // Close
    nvs_close(handle);
    
    return err;
}

esp_err_t wifi_creds_load(wifi_creds_t creds, const char* namespace) {
    wifi_creds_clear(creds);
    // // WIFI_CREDS_SET(creds, 0, "apucika", "mad12345");
    // // WIFI_CREDS_SET(creds, 1, "apucika_EXT", "mad12345");
    // WIFI_CREDS_SET(creds, 2, "Ulefone_S1", "dde33cc5ed6c");

    // Open
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace && namespace[0] ? namespace : MYESP_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    // Read
    size_t length = WIFI_CREDS_SIZE;
    err = nvs_get_blob(handle, "wifi_creds", creds, &length);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        wifi_creds_clear(creds);
        length = WIFI_CREDS_SIZE;
        err = ESP_OK;
    }
    if (err == ESP_OK) if (length != WIFI_CREDS_SIZE) err = ESP_FAIL;

    // Close
    nvs_close(handle);
    
    return err;
}

// ---------------------------------------------------------------
// FLASH
// ---------------------------------------------------------------

void flash_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

// ---------------------------------------------------------------
// GPIO
// ---------------------------------------------------------------

void gpio_pins_init(uint64_t _pin_bit_mask) {
    gpio_config_t io_conf = {
        .pin_bit_mask = _pin_bit_mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK( gpio_config(&io_conf) );
}

// ---------------------------------------------------------------
// WIFI APP
// ---------------------------------------------------------------


#define WIFI_SCAN_LIST_SIZE 10

typedef wifi_ap_record_t wifi_scan_list_t[WIFI_SCAN_LIST_SIZE];

void wifi_init() {
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
}


uint16_t wifi_scan(wifi_scan_list_t ap_info) {
    ESP_ERROR_CHECK(esp_wifi_start());

    uint16_t number = WIFI_SCAN_LIST_SIZE;
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(&ap_info));

    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);

    ESP_ERROR_CHECK( esp_wifi_stop() );
    return ap_count;
}


#define WIFI_CONNECT_MAXIMUM_RETRY  5

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

static int wifi_retry_num = 0;
bool wifi_disconnected = true;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "WIFI EVENT:%d", event_id);
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (wifi_retry_num < WIFI_CONNECT_MAXIMUM_RETRY) {
            esp_wifi_connect();
            wifi_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP (%d)", wifi_retry_num);
        } else {
            wifi_retry_num = 0;
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_retry_num = 0;
        wifi_disconnected = false;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_event_handler_instance_t wifi_handle_instance_disconnect;


static void wifi_disconnect_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) 
{
    ESP_LOGI(TAG, "WIFI DISCONNECT EVENT HANDLER:%d, TODO: reconnect...", event_id);
    ESP_ERROR_CHECK(
        esp_event_handler_instance_unregister(
            WIFI_EVENT, 
            WIFI_EVENT_STA_DISCONNECTED, 
            wifi_handle_instance_disconnect
        )
    );

    wifi_disconnected = true;
}

esp_err_t wifi_connect(const char* ssid, const char* pswd) {
    esp_err_t ret = ESP_OK;

    wifi_event_group = xEventGroupCreate();

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_STA_DISCONNECTED,
                                                        &wifi_disconnect_event_handler,
                                                        NULL,
                                                        &wifi_handle_instance_disconnect));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    strncpy((char*)wifi_config.sta.ssid, ssid, WIFI_SSID_SIZE);
    strncpy((char*)wifi_config.sta.password, pswd, WIFI_PSWD_SIZE);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 ssid, pswd);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 ssid, pswd);
        ret = ESP_ERR_WIFI_NOT_CONNECT;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        ret = ESP_FAIL;
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(wifi_event_group);
    return ret;
}

esp_err_t wifi_scan_connect(wifi_creds_t creds) {
    wifi_retry_num = 0;

    wifi_scan_list_t ap_info;
    uint16_t ap_count = wifi_scan(ap_info);

    for (int i = 0; (i < WIFI_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        // ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
        // //print_auth_mode(ap_info[i].authmode);
        // if (ap_info[i].authmode != WIFI_AUTH_WEP) {
        //     //print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        // }
        // ESP_LOGI(TAG, "Channel \t\t%d\n", ap_info[i].primary);

        for (int j=0; j<WIFI_CREDENTIALS; j++) {
            if (WIFI_CREDS_GET_SSID(creds, j)[0] && !strcmp(WIFI_CREDS_GET_SSID(creds, j), (char*)ap_info[i].ssid)) {
                ESP_LOGI(TAG, "cred found at (%d):'%s':'%s' => '%s'\n", i, 
                    WIFI_CREDS_GET_SSID(creds, j), 
                    WIFI_CREDS_GET_PSWD(creds, j), 
                    wifi_creds_get_pswd(creds,WIFI_CREDS_GET_SSID(creds, j))
                );
                if (ESP_OK == wifi_connect(WIFI_CREDS_GET_SSID(creds, j), WIFI_CREDS_GET_PSWD(creds, j))) return ESP_OK;
            }
        }
    }
    return ESP_ERR_WIFI_NOT_CONNECT;
}

typedef void (*wifi_app_cb_func_t)(void);

void wifi_connection_establish(wifi_creds_t creds, wifi_app_cb_func_t loop) {
    while(ESP_OK != wifi_scan_connect(creds)) {
        loop();
        ESP_LOGI(TAG, "WIFI connection failed. retry...\n");   
    }
    ESP_LOGI(TAG, "WIFI is now connected.\n"); 
}

void wifi_connection_keep_alive(wifi_creds_t creds, wifi_app_cb_func_t loop) {
    if (wifi_disconnected) wifi_connection_establish(creds, loop);
}


bool wifi_app_exited = false;

void wifi_app_run(wifi_creds_t creds, wifi_app_cb_func_t loop_while_connecting, wifi_app_cb_func_t loop_with_connection) {
    wifi_init();

    wifi_connection_establish(creds, loop_while_connecting);

    while(!wifi_app_exited) {
        wifi_connection_keep_alive(creds, loop_while_connecting);
        loop_with_connection();
    }
    ESP_LOGI(TAG, "\nOK, LEAVING...");
}

#define WIFI_APP_MODE_SETUP 0
#define WIFI_APP_MODE_RUN 1

int wifi_app_get_mode() {
    gpio_pins_init(1ULL<<GPIO_NUM_13);
    return gpio_get_level(GPIO_NUM_13) ? WIFI_APP_MODE_RUN : WIFI_APP_MODE_SETUP;
}

// --------------

/*

100 KEY
KF4GTX9
200 OK
100 SSID 10/1:
apucika
200 OK
100 PSWD 10/1:
mad12345
200 OK
100 SSID 10/2:
apucika_EXT
200 OK
100 PSWD 10/2:
mad12345
200 OK
100 SSID 10/3:

200 OK
100 PSWD 10/3:

....

200 OK Credentials are saved.

*/

// TODO: Unique code for each device. Should be changed before flash!
#define WIFI_APP_SETTINGS_KEY "KF4GTX9"

void wifi_app_setup(wifi_app_cb_func_t setup) {
    ESP_ERROR_CHECK( serial_install(UART_NUM_0, ESP_LINE_ENDINGS_CR, ESP_LINE_ENDINGS_CRLF) );

    printf("100 KEY\n");
    char* key = serial_readln();
    if (!strcmp(key, WIFI_APP_SETTINGS_KEY)) {
        free(key);

        wifi_creds_t creds;
        wifi_creds_clear(creds);

        for (int i=1; i<=WIFI_CREDENTIALS; i++) {
            printf("200 OK\n");

            printf("100 SSID %d/%d:\n", WIFI_CREDENTIALS, i);
            char* ssid = serial_readln();
            printf("200 OK\n");

            printf("100 PSWD %d/%d:\n", WIFI_CREDENTIALS, i);
            char* pswd = serial_readln();
            WIFI_CREDS_SET(creds, i, ssid, pswd);
            free(ssid);
            free(pswd);            
        }

        esp_err_t err = wifi_creds_save(creds, "camsys");
        ESP_OK == err ?             
            printf("200 OK Credentials are saved.\n") :
            printf("300 ERROR Saving credential error: %s\n", esp_err_to_name(err));

    } else {
        free(key);
        printf("300 ERROR\n");
    }

    ESP_ERROR_CHECK( serial_uninstall(UART_NUM_0) );
}

// --------------

void app_setup() {
    ESP_LOGI(TAG, "App setup when wifi setting just readed..\n");
    printf("app setup..\n");
    delay(1000);
}

void app_loop_while_connecting() {
    ESP_LOGI(TAG, "App working when no wifi..\n");
    printf("no wifi..\n");
    delay(1000);
}

void app_loop_with_connection() {
    ESP_LOGI(TAG, "App working when wifi connected..\n");
    printf("has wifi..\n");
    delay(1000);
}

void app_main(void)
{
    //esp_log_level_set("*", ESP_LOG_NONE);

    flash_init();
    
    wifi_creds_t creds;
    int mode = wifi_app_get_mode();
    switch (mode) {
        case WIFI_APP_MODE_SETUP:
            wifi_app_setup(app_setup);
            break;
        case WIFI_APP_MODE_RUN:

            ESP_ERROR_CHECK( wifi_creds_load(creds, "camsys") );

            wifi_app_run(creds, app_loop_while_connecting, app_loop_with_connection);
            break;
        default:
            ESP_LOGE(TAG, "Incorrect Wifi App Mode: %d", mode);
    }

    fflush(stdout);
    delay(1000);
    esp_restart();
}
