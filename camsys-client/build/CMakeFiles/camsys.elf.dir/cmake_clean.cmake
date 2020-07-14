file(REMOVE_RECURSE
  "bootloader/bootloader.bin"
  "bootloader/bootloader.elf"
  "bootloader/bootloader.map"
  "camsys.bin"
  "camsys.map"
  "config/sdkconfig.cmake"
  "config/sdkconfig.h"
  "flash_project_args"
  "project_elf_src.c"
  "CMakeFiles/camsys.elf.dir/project_elf_src.c.obj"
  "camsys.elf"
  "camsys.elf.pdb"
  "project_elf_src.c"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/camsys.elf.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
