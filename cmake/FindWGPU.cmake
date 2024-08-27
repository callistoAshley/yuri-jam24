find_library(
  WGPU_LIBRARY
  NAMES wgpu_native
  PATHS ${CMAKE_SOURCE_DIR}/vendor/wgpu-native/target/release
  REQUIRED
)
find_path(
  WGPU_INCLUDE_DIR
  NAMES wgpu.h
  PATHS ${CMAKE_SOURCE_DIR}/vendor/wgpu-native/ffi
  REQUIRED
)
