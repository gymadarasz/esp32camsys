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
CMAKE_SOURCE_DIR = /home/gyula/esp/esp-idf/components/bootloader/subproject

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/gyula/Desktop/esp32camsys/old/simple/build/bootloader

# Include any dependencies generated for this target.
include esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/depend.make

# Include the progress variables for this target.
include esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/progress.make

# Include the compile flags for this target's objects.
include esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/flags.make

esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_crc.c.obj: esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/flags.make
esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_crc.c.obj: /home/gyula/esp/esp-idf/components/esp_rom/patches/esp_rom_crc.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/gyula/Desktop/esp32camsys/old/simple/build/bootloader/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_crc.c.obj"
	cd /home/gyula/Desktop/esp32camsys/old/simple/build/bootloader/esp-idf/esp_rom && /home/gyula/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_crc.c.obj   -c /home/gyula/esp/esp-idf/components/esp_rom/patches/esp_rom_crc.c

esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_crc.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_crc.c.i"
	cd /home/gyula/Desktop/esp32camsys/old/simple/build/bootloader/esp-idf/esp_rom && /home/gyula/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/gyula/esp/esp-idf/components/esp_rom/patches/esp_rom_crc.c > CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_crc.c.i

esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_crc.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_crc.c.s"
	cd /home/gyula/Desktop/esp32camsys/old/simple/build/bootloader/esp-idf/esp_rom && /home/gyula/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin/xtensa-esp32-elf-gcc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/gyula/esp/esp-idf/components/esp_rom/patches/esp_rom_crc.c -o CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_crc.c.s

# Object files for target __idf_esp_rom
__idf_esp_rom_OBJECTS = \
"CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_crc.c.obj"

# External object files for target __idf_esp_rom
__idf_esp_rom_EXTERNAL_OBJECTS =

esp-idf/esp_rom/libesp_rom.a: esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/patches/esp_rom_crc.c.obj
esp-idf/esp_rom/libesp_rom.a: esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/build.make
esp-idf/esp_rom/libesp_rom.a: esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/gyula/Desktop/esp32camsys/old/simple/build/bootloader/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C static library libesp_rom.a"
	cd /home/gyula/Desktop/esp32camsys/old/simple/build/bootloader/esp-idf/esp_rom && $(CMAKE_COMMAND) -P CMakeFiles/__idf_esp_rom.dir/cmake_clean_target.cmake
	cd /home/gyula/Desktop/esp32camsys/old/simple/build/bootloader/esp-idf/esp_rom && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/__idf_esp_rom.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/build: esp-idf/esp_rom/libesp_rom.a

.PHONY : esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/build

esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/clean:
	cd /home/gyula/Desktop/esp32camsys/old/simple/build/bootloader/esp-idf/esp_rom && $(CMAKE_COMMAND) -P CMakeFiles/__idf_esp_rom.dir/cmake_clean.cmake
.PHONY : esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/clean

esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/depend:
	cd /home/gyula/Desktop/esp32camsys/old/simple/build/bootloader && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/gyula/esp/esp-idf/components/bootloader/subproject /home/gyula/esp/esp-idf/components/esp_rom /home/gyula/Desktop/esp32camsys/old/simple/build/bootloader /home/gyula/Desktop/esp32camsys/old/simple/build/bootloader/esp-idf/esp_rom /home/gyula/Desktop/esp32camsys/old/simple/build/bootloader/esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : esp-idf/esp_rom/CMakeFiles/__idf_esp_rom.dir/depend

