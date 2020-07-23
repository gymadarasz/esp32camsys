// #include "app_common.c"

/****************** CAMERA MODE *****************/


static camera_config_t recorder_camera_config = {
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

    .pixel_format = PIXFORMAT_JPEG, //PIXFORMAT_RGB888, //PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA, //FRAMESIZE_QVGA, //FRAMESIZE_UXGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 12, //0-63 lower number means higher quality
    .fb_count = 1 //if more than one, i2s runs in continuous mode. Use only with JPEG
};

// ------

struct recorder_state_s {
    bool streaming;
};

typedef struct recorder_state_s recorder_state_t;

#define RECORDER_STATE_DEFAULT { false }

recorder_state_t camera_state = RECORDER_STATE_DEFAULT;

// ----- httpd for camera recorder

#define CAMERA_PART_BOUNDARY "123456789000000000000987654321"
static const char* CAMERA__STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" CAMERA_PART_BOUNDARY;
static const char* CAMERA__STREAM_BOUNDARY = "\r\n--" CAMERA_PART_BOUNDARY "\r\n";
static const char* CAMERA__STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

esp_err_t uri_camera_stream_httpd_handler(httpd_req_t* req) {
    if (!check_secret(req)) return ESP_FAIL;

    camera_state.streaming = true;

    //camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;
    char * part_buf[64];
    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, CAMERA__STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    while(camera_state.streaming) {
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE("camsys", "Camera capture failed");
            res = ESP_FAIL;
            break;
        }

        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
        

        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, CAMERA__STREAM_BOUNDARY, strlen(CAMERA__STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, CAMERA__STREAM_PART, _jpg_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb->format != PIXFORMAT_JPEG){
            free(_jpg_buf);
        }
        esp_camera_fb_return(fb);
        if(res != ESP_OK){
            break;
        }
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        ESP_LOGI("camsys", "MJPG: %uKB %ums (%.1ffps)",
            (uint32_t)(_jpg_buf_len/1024),
            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
    }

    last_frame = 0;
    camera_state.streaming = false;
    return res;
}

static const httpd_uri_t uri_camera_stream = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = uri_camera_stream_httpd_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = NULL
};


// ----- camera app

void camera_additional_join_post_data(char* buff, size_t size) {
    snprintf(buff, size, 
        "type=camera&"
        "streaming=%d&", 
        camera_state.streaming
    );
}

// void camera_join_response_handler(char* buff, size_t size) {
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
//                 } else if (!strcmp(word_buff, "stopStream")) {
//                     get_str_piece_at(word_buff, word_size, line_buff, '=', 1);
//                     if (!settings_validate_required(word_buff, false) || !settings_validate_numeric(word_buff, false)) {
//                         PRINTF("StopStream parameter error - Invalid value: %s", word_buff);
//                     } else {
//                         if (atoi(word_buff) == 1) {
//                             PRINT("STOP STREAM REQUESTED IN JOIN REQUEST RESPONSE...");
//                             camera_state.streaming = false;
//                         } 
//                     }
//                 } else {
//                     PRINTF("Invalid property: '%s'", word_buff);
//                 }
//             }
//         }
        
//     }
// }


// void camera_setup(httpd_handle_t httpd_server) {

//     //power up the camera if PWDN pin is defined
//     if(CAM_PIN_PWDN != -1){
//         pinMode(CAM_PIN_PWDN, OUTPUT);
//         digitalWrite(CAM_PIN_PWDN, LOW);
//     }

//     //initialize the camera
//     ESP_ERROR_CHECK( esp_camera_init(&recorder_camera_config) );

//     PRINT("HTTPD init...");
//     //ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_server, &uri_camera_image));
//     ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_server, &uri_camera_stream));


// }

// void camera_setup_after_wifi(httpd_handle_t httpd_server) {

// }

// void camera_loop(void * pvParameters) {



//     PRINT("START camera_loop...");

//     while(1) {
//         if (!camera_state.streaming) {
//             PRINT("CAMERA IDLE - waiting for stream request...");
//             vTaskDelay(5000 / portTICK_RATE_MS);
//         }
//         // }
//     }

// }

// void camera_close() { }
