#include "sdkconfig.h"

#include "app_common.c"
#include "app_main_motion.c"
#include "app_main_camera.c"


#define APP_MODE_MOTION 1
#define APP_MODE_CAMERA 2

int app_mode = APP_MODE_MOTION;
// int app_mode = APP_MODE_CAMERA;

#define SWITCH_MODE(motion_code, camera_code) { \
    switch (app_mode) { \
        case APP_MODE_MOTION: \
            motion_code; \
        break; \
        case APP_MODE_CAMERA: \
            camera_code; \
        break; \
        default: ERROR("Wrong app mode"); \
    } \
}



void app_additional_join_post_data(char* buff, size_t size) {
    SWITCH_MODE(
        motion_additional_join_post_data(buff, size),
        camera_additional_join_post_data(buff, size)
    );
}

void app_join_response_handler(char* buff, size_t size) {
    PRINTF("RESPONSE STRING LENGTH: %d, BUFF_SIZE: %d\nRESPONE:\n%s", strlen(buff), size, buff);

    size_t line_size = 200;
    char line_buff[line_size];
    size_t word_size = 60;
    char word_buff[word_size];
    int lines = get_str_pieces(buff, '\n');
    for (int i=0; i<lines; i++) {
        if (get_str_piece_at(line_buff, line_size, buff, '\n', i)) {
            if (get_str_piece_at(word_buff, word_size, line_buff, '=', 0)) {                
                if (!strcmp(word_buff, "reset")) {
                    get_str_piece_at(word_buff, word_size, line_buff, '=', 1);
                    if (!settings_validate_required(word_buff, false) || !settings_validate_numeric(word_buff, false)) {
                        PRINTF("Reset parameter error - Invalid value: %s", word_buff);
                    } else {
                        if (atoi(word_buff) == 1) {
                            PRINT("RESET REQUESTED IN JOIN REQUEST RESPONSE...");
                            vTaskDelay(1000 / portTICK_RATE_MS);
                            esp_restart();
                        } 
                    }
                } else if (!strcmp(word_buff, "stopStream")) {
                    get_str_piece_at(word_buff, word_size, line_buff, '=', 1);
                    if (!settings_validate_required(word_buff, false) || !settings_validate_numeric(word_buff, false)) {
                        PRINTF("StopStream parameter error - Invalid value: %s", word_buff);
                    } else {
                        if (atoi(word_buff) == 1) {
                            PRINT("STOP STREAM REQUESTED IN JOIN REQUEST RESPONSE...");
                            camera_state.streaming = false;
                        } 
                    }
                } else if (!strcmp(word_buff, "watcher.x")) {
                    get_str_piece_at(word_buff, word_size, line_buff, '=', 1);
                    if (!settings_validate_required(word_buff, false) || !settings_validate_numeric(word_buff, false)) {
                        PRINTF("Watcher X should be a number. Invalid value: %s", word_buff);
                    } else {
                        motion_state.watcher.x = atoi(word_buff);
                    }
                } else if (!strcmp(word_buff, "watcher.y")) {
                    get_str_piece_at(word_buff, word_size, line_buff, '=', 1);
                    if (!settings_validate_required(word_buff, false) || !settings_validate_numeric(word_buff, false)) {
                        PRINTF("Watcher Y should be a number. Invalid value: %s", word_buff);
                    } else {
                        motion_state.watcher.y = atoi(word_buff);
                    }
                } else if (!strcmp(word_buff, "watcher.size")) {
                    get_str_piece_at(word_buff, word_size, line_buff, '=', 1);
                    if (!settings_validate_required(word_buff, false) || !settings_validate_numeric(word_buff, false)) {
                        PRINTF("Watcher Size should be a number. Invalid value: %s", word_buff);
                    } else {
                        motion_state.watcher.size = atoi(word_buff);
                    }
                } else if (!strcmp(word_buff, "watcher.raster")) {
                    get_str_piece_at(word_buff, word_size, line_buff, '=', 1);
                    if (!settings_validate_required(word_buff, false) || !settings_validate_numeric(word_buff, false)) {
                        PRINTF("Watcher Raster should be a number. Invalid value: %s", word_buff);
                    } else {
                        motion_state.watcher.raster = atoi(word_buff);
                    }
                } else if (!strcmp(word_buff, "watcher.threshold")) {
                    get_str_piece_at(word_buff, word_size, line_buff, '=', 1);
                    if (!settings_validate_required(word_buff, false) || !settings_validate_numeric(word_buff, false)) {
                        PRINTF("Watcher Threshold should be a number. Invalid value: %s", word_buff);
                    } else {
                        motion_state.watcher.threshold = atoi(word_buff);
                    }
                } else {
                    PRINTF("Invalid property: '%s'", word_buff);
                }
            }
        }
        
    }

    if (motion_state.diff_sum_max <= motion_state_last.diff_sum_max) motion_state.diff_sum_max = 0;
}

void app_setup(httpd_handle_t httpd_server) {
    
    //power up the camera if PWDN pin is defined
    if(CAM_PIN_PWDN != -1){
        pinMode(CAM_PIN_PWDN, OUTPUT);
        digitalWrite(CAM_PIN_PWDN, LOW);
    }

    //initialize the camera
    SWITCH_MODE(
        ESP_ERROR_CHECK( esp_camera_init(&motion_camera_config) ),
        ESP_ERROR_CHECK( esp_camera_init(&recorder_camera_config) )
    );
    
    // ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_server, &uri_motion_image));
    ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_server, &uri_motion_stream));
    ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_server, &uri_camera_stream));
}

void app_setup_after_wifi(httpd_handle_t httpd_server) { }

void app_loop(void * pvParameters) {

    //watch_t watcher = {43, 43, 10, 5, 100};
    static const int watcher_size_max = 40;
    static const size_t buff_size = watcher_size_max*watcher_size_max*4;
    uint8_t prev_buf[buff_size];

    while(1) {
        if (app_mode == APP_MODE_CAMERA) continue; // recorder camera does nothing

        fb = esp_camera_fb_get();            
        if (!fb) PRINT("Motion Camera capture failed");
        
        int xfrom = motion_state.watcher.x - motion_state.watcher.size;
        int xto = motion_state.watcher.x + motion_state.watcher.size;
        int yfrom = motion_state.watcher.y - motion_state.watcher.size;
        int yto = motion_state.watcher.y + motion_state.watcher.size;
        int i=0;
        size_t diff_sum = 0;
        for (int x=xfrom; x<xto; x+=motion_state.watcher.raster) {
            for (int y=yfrom; y<yto; y+=motion_state.watcher.raster) {
                int diff = fb->buf[x+y*fb->width] - prev_buf[i];
                diff_sum += (diff > 0 ? diff : -diff);
                if (i>=buff_size) {
                    PRINT("buff size too large");
                    break;
                }
                prev_buf[i] = fb->buf[x+y*fb->width];
                i++;
            }
        }

        if (motion_state.diff_sum_max < diff_sum) motion_state.diff_sum_max = diff_sum;
        
        bool alert = diff_sum >= motion_state.watcher.threshold;
        motion_show_diff(diff_sum, alert);
        if (alert) {
            // TODO: May we can switch here to record large screen?
            // PRINT("******************************************************");
            // PRINT("*********************** [ALERT] **********************");
            // PRINT("******************************************************");
        }
        
        esp_camera_fb_return(fb);
    }
}

void app_close() { }

void app_main() {
    stdio_init();

    nvs_handle_t nvs_handle;
    nvs_init(&nvs_handle);
    gpio_pins_init((1ULL<<GPIO_NUM_13));

    ESP_ERROR_CHECK( esp_netif_init() );

    httpd_handle_t httpd_server = httpd_server_init();
    ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_server, &uri_app_restart_request));

    const int app_tick_speed = 500;

    if (!gpio_get_level(GPIO_NUM_13)) {
        PRINT("APP STATE: SETUP");
        vTaskDelay(1000 / portTICK_RATE_MS);
        app_main_settings(nvs_handle);
    } else {
        PRINT("APP START");
        vTaskDelay(1000 / portTICK_RATE_MS);
        
        app_main_start(
            httpd_server, nvs_handle, app_tick_speed, 
            app_additional_join_post_data, app_join_response_handler, 
            app_setup, app_setup_after_wifi, app_loop, app_close
        );
    }

    nvs_close(nvs_handle);
    ESP_ERROR_CHECK( esp_netif_deinit() );
}
