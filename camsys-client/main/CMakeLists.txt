idf_component_register(SRCS "app_main.c"
                    INCLUDE_DIRS "."
                        lib/esp32-camera/driver
                        lib/esp32-camera/sensors
                        lib/esp32-camera/conversions
                        lib/esp32-camera/driver/include
                        lib/esp32-camera/conversions/include
                        lib/esp32-camera/driver/private_include
                        lib/esp32-camera/sensors/private_include
                        lib/esp32-camera/conversions/private_include)
					
set(COMPONENT_SRCS
  lib/esp32-camera/driver/camera.c
  lib/esp32-camera/driver/sccb.c
  lib/esp32-camera/driver/sensor.c
  lib/esp32-camera/driver/twi.c
  lib/esp32-camera/driver/xclk.c
  lib/esp32-camera/sensors/ov2640.c
  lib/esp32-camera/sensors/ov3660.c
  lib/esp32-camera/sensors/ov5640.c
  lib/esp32-camera/sensors/ov7725.c
  lib/esp32-camera/conversions/yuv.c
  lib/esp32-camera/conversions/to_jpg.cpp
  lib/esp32-camera/conversions/to_bmp.c
  lib/esp32-camera/conversions/jpge.cpp
  lib/esp32-camera/conversions/esp_jpg_decode.c
  )

set(COMPONENT_ADD_INCLUDEDIRS
  lib/esp32-camera/driver/include
  lib/esp32-camera/conversions/include
  )

set(COMPONENT_PRIV_INCLUDEDIRS
  lib/esp32-camera/driver/private_include
  lib/esp32-camera/sensors/private_include
  lib/esp32-camera/conversions/private_include
  )

set(COMPONENT_REQUIRES driver)
set(COMPONENT_PRIV_REQUIRES freertos nvs_flash)