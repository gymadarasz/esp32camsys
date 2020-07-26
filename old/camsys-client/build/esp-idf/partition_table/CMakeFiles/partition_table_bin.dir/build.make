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

# Utility rule file for partition_table_bin.

# Include the progress variables for this target.
include esp-idf/partition_table/CMakeFiles/partition_table_bin.dir/progress.make

esp-idf/partition_table/CMakeFiles/partition_table_bin: partition_table/partition-table.bin
esp-idf/partition_table/CMakeFiles/partition_table_bin: partition_table/partition-table.bin


partition_table/partition-table.bin: /home/gyula/esp/esp-idf/components/partition_table/partitions_singleapp.csv
partition_table/partition-table.bin: /home/gyula/esp/esp-idf/components/partition_table/gen_esp32part.py
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/gyula/Desktop/esp32camsys/camsys-client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating ../../partition_table/partition-table.bin"
	cd /home/gyula/Desktop/esp32camsys/camsys-client/build/esp-idf/partition_table && /home/gyula/.espressif/python_env/idf4.2_py3.8_env/bin/python /home/gyula/esp/esp-idf/components/partition_table/gen_esp32part.py -q --offset 0x8000 --flash-size 2MB /home/gyula/esp/esp-idf/components/partition_table/partitions_singleapp.csv /home/gyula/Desktop/esp32camsys/camsys-client/build/partition_table/partition-table.bin
	cd /home/gyula/Desktop/esp32camsys/camsys-client/build/esp-idf/partition_table && /usr/bin/cmake -E echo "Partition table binary generated. Contents:"
	cd /home/gyula/Desktop/esp32camsys/camsys-client/build/esp-idf/partition_table && /usr/bin/cmake -E echo "*******************************************************************************"
	cd /home/gyula/Desktop/esp32camsys/camsys-client/build/esp-idf/partition_table && /home/gyula/.espressif/python_env/idf4.2_py3.8_env/bin/python /home/gyula/esp/esp-idf/components/partition_table/gen_esp32part.py -q --offset 0x8000 --flash-size 2MB /home/gyula/Desktop/esp32camsys/camsys-client/build/partition_table/partition-table.bin
	cd /home/gyula/Desktop/esp32camsys/camsys-client/build/esp-idf/partition_table && /usr/bin/cmake -E echo "*******************************************************************************"

partition_table_bin: esp-idf/partition_table/CMakeFiles/partition_table_bin
partition_table_bin: partition_table/partition-table.bin
partition_table_bin: esp-idf/partition_table/CMakeFiles/partition_table_bin.dir/build.make

.PHONY : partition_table_bin

# Rule to build all files generated by this target.
esp-idf/partition_table/CMakeFiles/partition_table_bin.dir/build: partition_table_bin

.PHONY : esp-idf/partition_table/CMakeFiles/partition_table_bin.dir/build

esp-idf/partition_table/CMakeFiles/partition_table_bin.dir/clean:
	cd /home/gyula/Desktop/esp32camsys/camsys-client/build/esp-idf/partition_table && $(CMAKE_COMMAND) -P CMakeFiles/partition_table_bin.dir/cmake_clean.cmake
.PHONY : esp-idf/partition_table/CMakeFiles/partition_table_bin.dir/clean

esp-idf/partition_table/CMakeFiles/partition_table_bin.dir/depend:
	cd /home/gyula/Desktop/esp32camsys/camsys-client/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/gyula/Desktop/esp32camsys/camsys-client /home/gyula/esp/esp-idf/components/partition_table /home/gyula/Desktop/esp32camsys/camsys-client/build /home/gyula/Desktop/esp32camsys/camsys-client/build/esp-idf/partition_table /home/gyula/Desktop/esp32camsys/camsys-client/build/esp-idf/partition_table/CMakeFiles/partition_table_bin.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : esp-idf/partition_table/CMakeFiles/partition_table_bin.dir/depend
