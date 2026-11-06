add_library(pokketsx_libretro SHARED
  libretro_host.cpp
)

target_include_directories(pokketsx_libretro PRIVATE "${CMAKE_SOURCE_DIR}/dep/libretro-common/include")
target_link_libraries(pokketsx_libretro PRIVATE core common scmversion)

# libretro frontends look for cores by name, without a lib prefix.
set_target_properties(pokketsx_libretro PROPERTIES PREFIX "")
