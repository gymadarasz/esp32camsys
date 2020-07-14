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
