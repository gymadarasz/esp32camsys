// #include "app_common.c"

/****************** MOTION MODE *****************/

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


// void motion_init() {
    
//     //power up the camera if PWDN pin is defined
//     if(CAM_PIN_PWDN != -1){
//         pinMode(CAM_PIN_PWDN, OUTPUT);
//         digitalWrite(CAM_PIN_PWDN, LOW);
//     }

//     //initialize the camera
//     ESP_ERROR_CHECK( esp_camera_init(&motion_camera_config) );
// }

void motion_show_diff(size_t diff_sum, bool alert) {
    char spc[] = "]]]]]]]]]]]]]]]]]]]]]]]]]]]]][";
    char* s = spc; 
    for (int i=0; i<29; i++) {
        if (i*10>diff_sum) s[i] = ' ';
    }
    PRINTF("%s (%u) %s", s, diff_sum, alert ? "ALERT!!!" : "");
}

//----



struct watch_s {
    int x;
    int y;
    int size;
    int raster;
    int threshold;
};

typedef struct watch_s watch_t;

struct motion_state_s {
    size_t diff_sum_max;
    watch_t watcher;
};

#define WATCH_DEFAULT {43, 43, 10, 5, 250}

typedef struct motion_state_s motion_state_t;

motion_state_t motion_state_last = { 0, WATCH_DEFAULT };
motion_state_t motion_state = { 0, WATCH_DEFAULT };


void motion_additional_join_post_data(char* buff, size_t size) {
    // strncpy(buff, "type=motion", size);

    motion_state_last.diff_sum_max = motion_state.diff_sum_max; 

    snprintf(buff, size, 
        "type=motion&"
        "diff_sum_max=%d&"
        "watcher[x]=%d&"
        "watcher[y]=%d&"
        "watcher[size]=%d&"
        "watcher[raster]=%d&"
        "watcher[threshold]=%d&", 
        motion_state_last.diff_sum_max,
        motion_state.watcher.x,
        motion_state.watcher.y,
        motion_state.watcher.size,
        motion_state.watcher.raster,
        motion_state.watcher.threshold
    );

}

// void motion_join_response_handler(char* buff, size_t size) {
//     PRINTF("RESPONSE STRING LENGTH: %d, BUFF_SIZE: %d\nRESPONE:\n%s", strlen(buff), size, buff);

//     size_t line_size = 200;
//     char line_buff[line_size];
//     size_t word_size = 60;
//     char word_buff[word_size];
//     int lines = get_str_pieces(buff, '\n');
//     for (int i=0; i<lines; i++) {
//         if (get_str_piece_at(line_buff, line_size, buff, '\n', i)) {
//             if (get_str_piece_at(word_buff, word_size, line_buff, '=', 0)) {                
//                 if (!strcmp(word_buff, "reset")) {
//                     get_str_piece_at(word_buff, word_size, line_buff, '=', 1);
//                     if (!settings_validate_required(word_buff, false) || !settings_validate_numeric(word_buff, false)) {
//                         PRINTF("Reset parameter error - Invalid value: %s", word_buff);
//                     } else {
//                         if (atoi(word_buff) == 1) {
//                             PRINT("RESET REQUESTED IN JOIN REQUEST RESPONSE...");
//                             vTaskDelay(1000 / portTICK_RATE_MS);
//                             esp_restart();
//                         } 
//                     }
//                 } else if (!strcmp(word_buff, "watcher.x")) {
//                     get_str_piece_at(word_buff, word_size, line_buff, '=', 1);
//                     if (!settings_validate_required(word_buff, false) || !settings_validate_numeric(word_buff, false)) {
//                         PRINTF("Watcher X should be a number. Invalid value: %s", word_buff);
//                     } else {
//                         motion_state.watcher.x = atoi(word_buff);
//                     }
//                 } else if (!strcmp(word_buff, "watcher.y")) {
//                     get_str_piece_at(word_buff, word_size, line_buff, '=', 1);
//                     if (!settings_validate_required(word_buff, false) || !settings_validate_numeric(word_buff, false)) {
//                         PRINTF("Watcher Y should be a number. Invalid value: %s", word_buff);
//                     } else {
//                         motion_state.watcher.y = atoi(word_buff);
//                     }
//                 } else if (!strcmp(word_buff, "watcher.size")) {
//                     get_str_piece_at(word_buff, word_size, line_buff, '=', 1);
//                     if (!settings_validate_required(word_buff, false) || !settings_validate_numeric(word_buff, false)) {
//                         PRINTF("Watcher Size should be a number. Invalid value: %s", word_buff);
//                     } else {
//                         motion_state.watcher.size = atoi(word_buff);
//                     }
//                 } else if (!strcmp(word_buff, "watcher.raster")) {
//                     get_str_piece_at(word_buff, word_size, line_buff, '=', 1);
//                     if (!settings_validate_required(word_buff, false) || !settings_validate_numeric(word_buff, false)) {
//                         PRINTF("Watcher Raster should be a number. Invalid value: %s", word_buff);
//                     } else {
//                         motion_state.watcher.raster = atoi(word_buff);
//                     }
//                 } else if (!strcmp(word_buff, "watcher.threshold")) {
//                     get_str_piece_at(word_buff, word_size, line_buff, '=', 1);
//                     if (!settings_validate_required(word_buff, false) || !settings_validate_numeric(word_buff, false)) {
//                         PRINTF("Watcher Threshold should be a number. Invalid value: %s", word_buff);
//                     } else {
//                         motion_state.watcher.threshold = atoi(word_buff);
//                     }
//                 } else {
//                     PRINTF("Invalid property: '%s'", word_buff);
//                 }
//             }
//         }
        
//     }

//     if (motion_state.diff_sum_max <= motion_state_last.diff_sum_max) motion_state.diff_sum_max = 0;
// }

// ----- httpd ------

#define MOTION_PART_BOUNDARY "123456789000000000000987654321"
static const char* MOTION__STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" MOTION_PART_BOUNDARY;
static const char* MOTION__STREAM_BOUNDARY = "\r\n--" MOTION_PART_BOUNDARY "\r\n";
static const char* MOTION__STREAM_PART = "Content-Type: image/x-windows-bmp\r\nContent-Length: %u\r\n\r\n";

esp_err_t uri_motion_stream_httpd_handler(httpd_req_t* req) {
    if (!check_secret(req)) return ESP_FAIL;

    // camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _bmp_buf_len;
    uint8_t * _bmp_buf;
    char * part_buf[64];

    res = httpd_resp_set_type(req, MOTION__STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    while(true){
        if (!fb) {
            fb = esp_camera_fb_get();
        }
        if (!fb) {
            ESP_LOGE("camsys", "Camera capture failed");
            res = ESP_FAIL;
            break;
        }
        
        bool bmp_converted = frame2bmp(fb, &_bmp_buf, &_bmp_buf_len);
        if(!bmp_converted){
            ESP_LOGE("camsys", "BMP compression failed");
            esp_camera_fb_return(fb);
            res = ESP_FAIL;
        }


        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, MOTION__STREAM_BOUNDARY, strlen(MOTION__STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, MOTION__STREAM_PART, _bmp_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_bmp_buf, _bmp_buf_len);
        }
        if(fb->format != PIXFORMAT_JPEG){
            free(_bmp_buf);
        }
        esp_camera_fb_return(fb);
        if(res != ESP_OK){
            break;
        }
    }

    return res;
}

// static const httpd_uri_t uri_motion_image = {
//     .uri       = "/image",
//     .method    = HTTP_GET,
//     .handler   = uri_motion_image_httpd_handler, // TODO: JPEG FASTER!! use/convert jpeg at motion monitor to mark fixed pixels on screen to get faster net speeed
//     /* Let's pass response string in user
//      * context to demonstrate it's usage */
//     .user_ctx  = NULL
// };

static const httpd_uri_t uri_motion_stream = {
    .uri       = "/stream-bmp",
    .method    = HTTP_GET,
    .handler   = uri_motion_stream_httpd_handler, // TODO: JPEG FASTER!! use/convert jpeg at motion monitor to mark fixed pixels on screen to get faster net speeed
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};



// void motion_setup(httpd_handle_t httpd_server) {
    
//     //power up the camera if PWDN pin is defined
//     if(CAM_PIN_PWDN != -1){
//         pinMode(CAM_PIN_PWDN, OUTPUT);
//         digitalWrite(CAM_PIN_PWDN, LOW);
//     }

//     //initialize the camera
//     ESP_ERROR_CHECK( esp_camera_init(&motion_camera_config) );

//     // ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_server, &uri_motion_image));
//     ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_server, &uri_motion_stream));
// }

// void motion_setup_after_wifi(httpd_handle_t httpd_server) { }

// void motion_loop(void * pvParameters) {

//     PRINT("START motion_loop...");

//     //watch_t watcher = {43, 43, 10, 5, 100};
//     static const int watcher_size_max = 40;
//     static const size_t buff_size = watcher_size_max*watcher_size_max*4;
//     uint8_t prev_buf[buff_size];

//     while(1) {

//         fb = esp_camera_fb_get();            
//         if (!fb) PRINT("Motion Camera capture failed");
        
//         int xfrom = motion_state.watcher.x - motion_state.watcher.size;
//         int xto = motion_state.watcher.x + motion_state.watcher.size;
//         int yfrom = motion_state.watcher.y - motion_state.watcher.size;
//         int yto = motion_state.watcher.y + motion_state.watcher.size;
//         int i=0;
//         size_t diff_sum = 0;
//         for (int x=xfrom; x<xto; x+=motion_state.watcher.raster) {
//             for (int y=yfrom; y<yto; y+=motion_state.watcher.raster) {
//                 int diff = fb->buf[x+y*fb->width] - prev_buf[i];
//                 diff_sum += (diff > 0 ? diff : -diff);
//                 if (i>=buff_size) {
//                     PRINT("buff size too large");
//                     break;
//                 }
//                 prev_buf[i] = fb->buf[x+y*fb->width];
//                 i++;
//             }
//         }

//         if (motion_state.diff_sum_max < diff_sum) motion_state.diff_sum_max = diff_sum;
        
//         bool alert = diff_sum >= motion_state.watcher.threshold;
//         motion_show_diff(diff_sum, alert);
//         if (alert) {
//             // PRINT("******************************************************");
//             // PRINT("*********************** [ALERT] **********************");
//             // PRINT("******************************************************");
//         }
        
//         esp_camera_fb_return(fb);
//     }
// }

// void motion_close() { }
