# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/gyula/Desktop/esp32camsys/camsys-client

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/gyula/Desktop/esp32camsys/camsys-client/build

# Include any dependencies generated for this target.
include CMakeFiles/camsys.elf.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/camsys.elf.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/camsys.elf.dir/flags.make

project_elf_src.c:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/gyula/Desktop/esp32camsys/camsys-client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating project_elf_src.c"
	/usr/bin/cmake -E touch /home/gyula/Desktop/esp32camsys/camsys-client/build/project_elf_src.c

CMakeFiles/camsys.elf.dir/project_elf_src.c.obj: CMakeFiles/camsys.elf.dir/flags.make
CMakeFiles/camsys.elf.dir/project_elf_src.c.obj: project_elf_src.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/gyula/Desktop/esp32camsys/camsys-client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/camsys.elf.dir/project_elf_src.c.obj"
	/home/gyula/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/camsys.elf.dir/project_elf_src.c.obj   -c /home/gyula/Desktop/esp32camsys/camsys-client/build/project_elf_src.c

CMakeFiles/camsys.elf.dir/project_elf_src.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/camsys.elf.dir/project_elf_src.c.i"
	/home/gyula/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/gyula/Desktop/esp32camsys/camsys-client/build/project_elf_src.c > CMakeFiles/camsys.elf.dir/project_elf_src.c.i

CMakeFiles/camsys.elf.dir/project_elf_src.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/camsys.elf.dir/project_elf_src.c.s"
	/home/gyula/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/gyula/Desktop/esp32camsys/camsys-client/build/project_elf_src.c -o CMakeFiles/camsys.elf.dir/project_elf_src.c.s

# Object files for target camsys.elf
camsys_elf_OBJECTS = \
"CMakeFiles/camsys.elf.dir/project_elf_src.c.obj"

# External object files for target camsys.elf
camsys_elf_EXTERNAL_OBJECTS =

camsys.elf: CMakeFiles/camsys.elf.dir/project_elf_src.c.obj
camsys.elf: CMakeFiles/camsys.elf.dir/build.make
camsys.elf: esp-idf/xtensa/libxtensa.a
camsys.elf: esp-idf/mbedtls/libmbedtls.a
camsys.elf: esp-idf/efuse/libefuse.a
camsys.elf: esp-idf/bootloader_support/libbootloader_support.a
camsys.elf: esp-idf/app_update/libapp_update.a
camsys.elf: esp-idf/esp_ipc/libesp_ipc.a
camsys.elf: esp-idf/spi_flash/libspi_flash.a
camsys.elf: esp-idf/nvs_flash/libnvs_flash.a
camsys.elf: esp-idf/pthread/libpthread.a
camsys.elf: esp-idf/esp_system/libesp_system.a
camsys.elf: esp-idf/esp_rom/libesp_rom.a
camsys.elf: esp-idf/soc/libsoc.a
camsys.elf: esp-idf/vfs/libvfs.a
camsys.elf: esp-idf/esp_eth/libesp_eth.a
camsys.elf: esp-idf/tcpip_adapter/libtcpip_adapter.a
camsys.elf: esp-idf/esp_netif/libesp_netif.a
camsys.elf: esp-idf/esp_event/libesp_event.a
camsys.elf: esp-idf/wpa_supplicant/libwpa_supplicant.a
camsys.elf: esp-idf/esp_wifi/libesp_wifi.a
camsys.elf: esp-idf/lwip/liblwip.a
camsys.elf: esp-idf/log/liblog.a
camsys.elf: esp-idf/heap/libheap.a
camsys.elf: esp-idf/esp_ringbuf/libesp_ringbuf.a
camsys.elf: esp-idf/driver/libdriver.a
camsys.elf: esp-idf/espcoredump/libespcoredump.a
camsys.elf: esp-idf/perfmon/libperfmon.a
camsys.elf: esp-idf/esp32/libesp32.a
camsys.elf: esp-idf/esp_common/libesp_common.a
camsys.elf: esp-idf/esp_timer/libesp_timer.a
camsys.elf: esp-idf/freertos/libfreertos.a
camsys.elf: esp-idf/newlib/libnewlib.a
camsys.elf: esp-idf/cxx/libcxx.a
camsys.elf: esp-idf/app_trace/libapp_trace.a
camsys.elf: esp-idf/asio/libasio.a
camsys.elf: esp-idf/cbor/libcbor.a
camsys.elf: esp-idf/coap/libcoap.a
camsys.elf: esp-idf/console/libconsole.a
camsys.elf: esp-idf/nghttp/libnghttp.a
camsys.elf: esp-idf/esp-tls/libesp-tls.a
camsys.elf: esp-idf/esp_adc_cal/libesp_adc_cal.a
camsys.elf: esp-idf/esp_gdbstub/libesp_gdbstub.a
camsys.elf: esp-idf/esp_hid/libesp_hid.a
camsys.elf: esp-idf/tcp_transport/libtcp_transport.a
camsys.elf: esp-idf/esp_http_client/libesp_http_client.a
camsys.elf: esp-idf/esp_http_server/libesp_http_server.a
camsys.elf: esp-idf/esp_https_ota/libesp_https_ota.a
camsys.elf: esp-idf/protobuf-c/libprotobuf-c.a
camsys.elf: esp-idf/protocomm/libprotocomm.a
camsys.elf: esp-idf/mdns/libmdns.a
camsys.elf: esp-idf/esp_local_ctrl/libesp_local_ctrl.a
camsys.elf: esp-idf/sdmmc/libsdmmc.a
camsys.elf: esp-idf/esp_serial_slave_link/libesp_serial_slave_link.a
camsys.elf: esp-idf/esp_websocket_client/libesp_websocket_client.a
camsys.elf: esp-idf/expat/libexpat.a
camsys.elf: esp-idf/wear_levelling/libwear_levelling.a
camsys.elf: esp-idf/fatfs/libfatfs.a
camsys.elf: esp-idf/freemodbus/libfreemodbus.a
camsys.elf: esp-idf/jsmn/libjsmn.a
camsys.elf: esp-idf/json/libjson.a
camsys.elf: esp-idf/libsodium/liblibsodium.a
camsys.elf: esp-idf/mqtt/libmqtt.a
camsys.elf: esp-idf/openssl/libopenssl.a
camsys.elf: esp-idf/spiffs/libspiffs.a
camsys.elf: esp-idf/ulp/libulp.a
camsys.elf: esp-idf/unity/libunity.a
camsys.elf: esp-idf/wifi_provisioning/libwifi_provisioning.a
camsys.elf: esp-idf/protocol_examples_common/libprotocol_examples_common.a
camsys.elf: esp-idf/main/libmain.a
camsys.elf: esp-idf/asio/libasio.a
camsys.elf: esp-idf/cbor/libcbor.a
camsys.elf: esp-idf/coap/libcoap.a
camsys.elf: esp-idf/esp_adc_cal/libesp_adc_cal.a
camsys.elf: esp-idf/esp_gdbstub/libesp_gdbstub.a
camsys.elf: esp-idf/esp_hid/libesp_hid.a
camsys.elf: esp-idf/esp_https_ota/libesp_https_ota.a
camsys.elf: esp-idf/esp_local_ctrl/libesp_local_ctrl.a
camsys.elf: esp-idf/esp_serial_slave_link/libesp_serial_slave_link.a
camsys.elf: esp-idf/esp_websocket_client/libesp_websocket_client.a
camsys.elf: esp-idf/expat/libexpat.a
camsys.elf: esp-idf/fatfs/libfatfs.a
camsys.elf: esp-idf/sdmmc/libsdmmc.a
camsys.elf: esp-idf/wear_levelling/libwear_levelling.a
camsys.elf: esp-idf/freemodbus/libfreemodbus.a
camsys.elf: esp-idf/jsmn/libjsmn.a
camsys.elf: esp-idf/libsodium/liblibsodium.a
camsys.elf: esp-idf/mqtt/libmqtt.a
camsys.elf: esp-idf/openssl/libopenssl.a
camsys.elf: esp-idf/spiffs/libspiffs.a
camsys.elf: esp-idf/unity/libunity.a
camsys.elf: esp-idf/wifi_provisioning/libwifi_provisioning.a
camsys.elf: esp-idf/protocomm/libprotocomm.a
camsys.elf: esp-idf/protobuf-c/libprotobuf-c.a
camsys.elf: esp-idf/mdns/libmdns.a
camsys.elf: esp-idf/console/libconsole.a
camsys.elf: esp-idf/json/libjson.a
camsys.elf: esp-idf/protocol_examples_common/libprotocol_examples_common.a
camsys.elf: esp-idf/xtensa/libxtensa.a
camsys.elf: esp-idf/mbedtls/libmbedtls.a
camsys.elf: esp-idf/efuse/libefuse.a
camsys.elf: esp-idf/bootloader_support/libbootloader_support.a
camsys.elf: esp-idf/app_update/libapp_update.a
camsys.elf: esp-idf/esp_ipc/libesp_ipc.a
camsys.elf: esp-idf/spi_flash/libspi_flash.a
camsys.elf: esp-idf/nvs_flash/libnvs_flash.a
camsys.elf: esp-idf/pthread/libpthread.a
camsys.elf: esp-idf/esp_system/libesp_system.a
camsys.elf: esp-idf/esp_rom/libesp_rom.a
camsys.elf: esp-idf/soc/libsoc.a
camsys.elf: esp-idf/vfs/libvfs.a
camsys.elf: esp-idf/esp_eth/libesp_eth.a
camsys.elf: esp-idf/tcpip_adapter/libtcpip_adapter.a
camsys.elf: esp-idf/esp_netif/libesp_netif.a
camsys.elf: esp-idf/esp_event/libesp_event.a
camsys.elf: esp-idf/wpa_supplicant/libwpa_supplicant.a
camsys.elf: esp-idf/esp_wifi/libesp_wifi.a
camsys.elf: esp-idf/lwip/liblwip.a
camsys.elf: esp-idf/log/liblog.a
camsys.elf: esp-idf/heap/libheap.a
camsys.elf: esp-idf/esp_ringbuf/libesp_ringbuf.a
camsys.elf: esp-idf/driver/libdriver.a
camsys.elf: esp-idf/espcoredump/libespcoredump.a
camsys.elf: esp-idf/perfmon/libperfmon.a
camsys.elf: esp-idf/esp32/libesp32.a
camsys.elf: esp-idf/esp_common/libesp_common.a
camsys.elf: esp-idf/esp_timer/libesp_timer.a
camsys.elf: esp-idf/freertos/libfreertos.a
camsys.elf: esp-idf/newlib/libnewlib.a
camsys.elf: esp-idf/cxx/libcxx.a
camsys.elf: esp-idf/app_trace/libapp_trace.a
camsys.elf: esp-idf/nghttp/libnghttp.a
camsys.elf: esp-idf/esp-tls/libesp-tls.a
camsys.elf: esp-idf/tcp_transport/libtcp_transport.a
camsys.elf: esp-idf/esp_http_client/libesp_http_client.a
camsys.elf: esp-idf/esp_http_server/libesp_http_server.a
camsys.elf: esp-idf/ulp/libulp.a
camsys.elf: esp-idf/mbedtls/mbedtls/library/libmbedtls.a
camsys.elf: esp-idf/mbedtls/mbedtls/library/libmbedcrypto.a
camsys.elf: esp-idf/mbedtls/mbedtls/library/libmbedx509.a
camsys.elf: esp-idf/soc/soc/esp32/libsoc_esp32.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libcoexist.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libcore.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libespnow.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libmesh.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libnet80211.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libpp.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/librtc.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libsmartconfig.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libphy.a
camsys.elf: esp-idf/xtensa/libxtensa.a
camsys.elf: esp-idf/mbedtls/libmbedtls.a
camsys.elf: esp-idf/efuse/libefuse.a
camsys.elf: esp-idf/bootloader_support/libbootloader_support.a
camsys.elf: esp-idf/app_update/libapp_update.a
camsys.elf: esp-idf/esp_ipc/libesp_ipc.a
camsys.elf: esp-idf/spi_flash/libspi_flash.a
camsys.elf: esp-idf/nvs_flash/libnvs_flash.a
camsys.elf: esp-idf/pthread/libpthread.a
camsys.elf: esp-idf/esp_system/libesp_system.a
camsys.elf: esp-idf/esp_rom/libesp_rom.a
camsys.elf: esp-idf/soc/libsoc.a
camsys.elf: esp-idf/vfs/libvfs.a
camsys.elf: esp-idf/esp_eth/libesp_eth.a
camsys.elf: esp-idf/tcpip_adapter/libtcpip_adapter.a
camsys.elf: esp-idf/esp_netif/libesp_netif.a
camsys.elf: esp-idf/esp_event/libesp_event.a
camsys.elf: esp-idf/wpa_supplicant/libwpa_supplicant.a
camsys.elf: esp-idf/esp_wifi/libesp_wifi.a
camsys.elf: esp-idf/lwip/liblwip.a
camsys.elf: esp-idf/log/liblog.a
camsys.elf: esp-idf/heap/libheap.a
camsys.elf: esp-idf/esp_ringbuf/libesp_ringbuf.a
camsys.elf: esp-idf/driver/libdriver.a
camsys.elf: esp-idf/espcoredump/libespcoredump.a
camsys.elf: esp-idf/perfmon/libperfmon.a
camsys.elf: esp-idf/esp32/libesp32.a
camsys.elf: esp-idf/esp_common/libesp_common.a
camsys.elf: esp-idf/esp_timer/libesp_timer.a
camsys.elf: esp-idf/freertos/libfreertos.a
camsys.elf: esp-idf/newlib/libnewlib.a
camsys.elf: esp-idf/cxx/libcxx.a
camsys.elf: esp-idf/app_trace/libapp_trace.a
camsys.elf: esp-idf/nghttp/libnghttp.a
camsys.elf: esp-idf/esp-tls/libesp-tls.a
camsys.elf: esp-idf/tcp_transport/libtcp_transport.a
camsys.elf: esp-idf/esp_http_client/libesp_http_client.a
camsys.elf: esp-idf/esp_http_server/libesp_http_server.a
camsys.elf: esp-idf/ulp/libulp.a
camsys.elf: esp-idf/mbedtls/mbedtls/library/libmbedtls.a
camsys.elf: esp-idf/mbedtls/mbedtls/library/libmbedcrypto.a
camsys.elf: esp-idf/mbedtls/mbedtls/library/libmbedx509.a
camsys.elf: esp-idf/soc/soc/esp32/libsoc_esp32.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libcoexist.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libcore.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libespnow.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libmesh.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libnet80211.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libpp.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/librtc.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libsmartconfig.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libphy.a
camsys.elf: esp-idf/xtensa/libxtensa.a
camsys.elf: esp-idf/mbedtls/libmbedtls.a
camsys.elf: esp-idf/efuse/libefuse.a
camsys.elf: esp-idf/bootloader_support/libbootloader_support.a
camsys.elf: esp-idf/app_update/libapp_update.a
camsys.elf: esp-idf/esp_ipc/libesp_ipc.a
camsys.elf: esp-idf/spi_flash/libspi_flash.a
camsys.elf: esp-idf/nvs_flash/libnvs_flash.a
camsys.elf: esp-idf/pthread/libpthread.a
camsys.elf: esp-idf/esp_system/libesp_system.a
camsys.elf: esp-idf/esp_rom/libesp_rom.a
camsys.elf: esp-idf/soc/libsoc.a
camsys.elf: esp-idf/vfs/libvfs.a
camsys.elf: esp-idf/esp_eth/libesp_eth.a
camsys.elf: esp-idf/tcpip_adapter/libtcpip_adapter.a
camsys.elf: esp-idf/esp_netif/libesp_netif.a
camsys.elf: esp-idf/esp_event/libesp_event.a
camsys.elf: esp-idf/wpa_supplicant/libwpa_supplicant.a
camsys.elf: esp-idf/esp_wifi/libesp_wifi.a
camsys.elf: esp-idf/lwip/liblwip.a
camsys.elf: esp-idf/log/liblog.a
camsys.elf: esp-idf/heap/libheap.a
camsys.elf: esp-idf/esp_ringbuf/libesp_ringbuf.a
camsys.elf: esp-idf/driver/libdriver.a
camsys.elf: esp-idf/espcoredump/libespcoredump.a
camsys.elf: esp-idf/perfmon/libperfmon.a
camsys.elf: esp-idf/esp32/libesp32.a
camsys.elf: esp-idf/esp_common/libesp_common.a
camsys.elf: esp-idf/esp_timer/libesp_timer.a
camsys.elf: esp-idf/freertos/libfreertos.a
camsys.elf: esp-idf/newlib/libnewlib.a
camsys.elf: esp-idf/cxx/libcxx.a
camsys.elf: esp-idf/app_trace/libapp_trace.a
camsys.elf: esp-idf/nghttp/libnghttp.a
camsys.elf: esp-idf/esp-tls/libesp-tls.a
camsys.elf: esp-idf/tcp_transport/libtcp_transport.a
camsys.elf: esp-idf/esp_http_client/libesp_http_client.a
camsys.elf: esp-idf/esp_http_server/libesp_http_server.a
camsys.elf: esp-idf/ulp/libulp.a
camsys.elf: esp-idf/mbedtls/mbedtls/library/libmbedtls.a
camsys.elf: esp-idf/mbedtls/mbedtls/library/libmbedcrypto.a
camsys.elf: esp-idf/mbedtls/mbedtls/library/libmbedx509.a
camsys.elf: esp-idf/soc/soc/esp32/libsoc_esp32.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libcoexist.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libcore.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libespnow.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libmesh.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libnet80211.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libpp.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/librtc.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libsmartconfig.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libphy.a
camsys.elf: esp-idf/xtensa/libxtensa.a
camsys.elf: esp-idf/mbedtls/libmbedtls.a
camsys.elf: esp-idf/efuse/libefuse.a
camsys.elf: esp-idf/bootloader_support/libbootloader_support.a
camsys.elf: esp-idf/app_update/libapp_update.a
camsys.elf: esp-idf/esp_ipc/libesp_ipc.a
camsys.elf: esp-idf/spi_flash/libspi_flash.a
camsys.elf: esp-idf/nvs_flash/libnvs_flash.a
camsys.elf: esp-idf/pthread/libpthread.a
camsys.elf: esp-idf/esp_system/libesp_system.a
camsys.elf: esp-idf/esp_rom/libesp_rom.a
camsys.elf: esp-idf/soc/libsoc.a
camsys.elf: esp-idf/vfs/libvfs.a
camsys.elf: esp-idf/esp_eth/libesp_eth.a
camsys.elf: esp-idf/tcpip_adapter/libtcpip_adapter.a
camsys.elf: esp-idf/esp_netif/libesp_netif.a
camsys.elf: esp-idf/esp_event/libesp_event.a
camsys.elf: esp-idf/wpa_supplicant/libwpa_supplicant.a
camsys.elf: esp-idf/esp_wifi/libesp_wifi.a
camsys.elf: esp-idf/lwip/liblwip.a
camsys.elf: esp-idf/log/liblog.a
camsys.elf: esp-idf/heap/libheap.a
camsys.elf: esp-idf/esp_ringbuf/libesp_ringbuf.a
camsys.elf: esp-idf/driver/libdriver.a
camsys.elf: esp-idf/espcoredump/libespcoredump.a
camsys.elf: esp-idf/perfmon/libperfmon.a
camsys.elf: esp-idf/esp32/libesp32.a
camsys.elf: esp-idf/esp_common/libesp_common.a
camsys.elf: esp-idf/esp_timer/libesp_timer.a
camsys.elf: esp-idf/freertos/libfreertos.a
camsys.elf: esp-idf/newlib/libnewlib.a
camsys.elf: esp-idf/cxx/libcxx.a
camsys.elf: esp-idf/app_trace/libapp_trace.a
camsys.elf: esp-idf/nghttp/libnghttp.a
camsys.elf: esp-idf/esp-tls/libesp-tls.a
camsys.elf: esp-idf/tcp_transport/libtcp_transport.a
camsys.elf: esp-idf/esp_http_client/libesp_http_client.a
camsys.elf: esp-idf/esp_http_server/libesp_http_server.a
camsys.elf: esp-idf/ulp/libulp.a
camsys.elf: esp-idf/mbedtls/mbedtls/library/libmbedtls.a
camsys.elf: esp-idf/mbedtls/mbedtls/library/libmbedcrypto.a
camsys.elf: esp-idf/mbedtls/mbedtls/library/libmbedx509.a
camsys.elf: esp-idf/soc/soc/esp32/libsoc_esp32.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libcoexist.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libcore.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libespnow.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libmesh.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libnet80211.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libpp.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/librtc.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libsmartconfig.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_wifi/lib/esp32/libphy.a
camsys.elf: /home/gyula/esp/esp-idf/components/xtensa/esp32/libhal.a
camsys.elf: esp-idf/newlib/libnewlib.a
camsys.elf: esp-idf/pthread/libpthread.a
camsys.elf: esp-idf/app_trace/libapp_trace.a
camsys.elf: esp-idf/app_trace/libapp_trace.a
camsys.elf: /home/gyula/esp/esp-idf/components/esp_rom/esp32/ld/esp32.rom.api.ld
camsys.elf: /home/gyula/esp/esp-idf/components/esp_rom/esp32/ld/esp32.rom.ld
camsys.elf: /home/gyula/esp/esp-idf/components/esp_rom/esp32/ld/esp32.rom.libgcc.ld
camsys.elf: /home/gyula/esp/esp-idf/components/esp_rom/esp32/ld/esp32.rom.newlib-data.ld
camsys.elf: /home/gyula/esp/esp-idf/components/esp_rom/esp32/ld/esp32.rom.syscalls.ld
camsys.elf: esp-idf/esp32/esp32_out.ld
camsys.elf: esp-idf/esp32/ld/esp32.project.ld
camsys.elf: /home/gyula/esp/esp-idf/components/esp32/ld/esp32.peripherals.ld
camsys.elf: CMakeFiles/camsys.elf.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/gyula/Desktop/esp32camsys/camsys-client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable camsys.elf"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/camsys.elf.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/camsys.elf.dir/build: camsys.elf

.PHONY : CMakeFiles/camsys.elf.dir/build

CMakeFiles/camsys.elf.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/camsys.elf.dir/cmake_clean.cmake
.PHONY : CMakeFiles/camsys.elf.dir/clean

CMakeFiles/camsys.elf.dir/depend: project_elf_src.c
	cd /home/gyula/Desktop/esp32camsys/camsys-client/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/gyula/Desktop/esp32camsys/camsys-client /home/gyula/Desktop/esp32camsys/camsys-client /home/gyula/Desktop/esp32camsys/camsys-client/build /home/gyula/Desktop/esp32camsys/camsys-client/build /home/gyula/Desktop/esp32camsys/camsys-client/build/CMakeFiles/camsys.elf.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/camsys.elf.dir/depend
