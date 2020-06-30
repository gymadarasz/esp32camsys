//#include <stdio.h>
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "driver/gpio.h"
//#include "sdkconfig.h"


#include <esp_event.h>
#include <esp_log.h>

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

static camera_config_t camera_config = {
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
    char s[] = "                              ";
    for (int i=0; i<29; i++) {
        if (i*10>diff_sum) s[i] = "X";
    }
    ESP_LOGI(TAG, "%s", s);
}

void app_main()
{
    size_t raster = 3000;
    size_t buff_size = 9216/raster;
    uint8_t prev_buf[buff_size];
    
    ESP_LOGI(TAG, "******INIT*******");
        
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        app_error(err);
    }   
    ESP_LOGI(TAG, "******INIT OK*******");
    
    
    while(1) {
        //vTaskDelay(1000 / portTICK_PERIOD_MS); 
        
        int64_t fr_start = esp_timer_get_time();
        
        //ESP_LOGI(TAG, "******CAPTURE*******");
        camera_fb_t * fb = esp_camera_fb_get();            
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            app_warn(ESP_FAIL);
        }
        //ESP_LOGI(TAG, "FRAME BUFFER: %u %u %u", fb->len, fb->width, fb->height);
        
        
        //ESP_LOGI(TAG, "******DIFF*******");
        size_t diff_sum = 0;
        for (size_t i=0; i<fb->len/raster; i++) {
            int diff = fb->buf[i*raster] - prev_buf[i];
//            ESP_LOGI(TAG, "DIFF: %u, (%u %u)", diff, fb->buf[i], prev_buf[i]);
            diff_sum += (diff > 0 ? diff : -diff);
            prev_buf[i] = fb->buf[i*raster];       
        }
        //ESP_LOGI(TAG, "DIFF SUM: %u", diff_sum);
        show_diff(diff_sum);
        
        esp_camera_fb_return(fb);
    
        int64_t fr_end = esp_timer_get_time();
        //ESP_LOGI(TAG, "Loop: %ums", (uint32_t)((fr_end - fr_start)/1000));
    }
}
