cmake_minimum_required(VERSION 3.16)

project(DvlGfx VERSION 1.0.0 LANGUAGES C CXX)

option(ASAN "Enable address sanitizer" ON)
option(UBSAN "Enable undefined behaviour sanitizer" ON)
option(ENABLE_INSTALL "Enable install targets" ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

if(CMAKE_BUILD_TYPE MATCHES "Release")
  set(ASAN OFF)
  set(UBSAN OFF)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clang-tidy

add_library(common INTERFACE)
target_include_directories(common INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/public/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/tl>
  $<INSTALL_INTERFACE:include>)
set_target_properties(common PROPERTIES PUBLIC_HEADER "src/public/include/dvl_gfx_common.hpp")
set_target_properties(common PROPERTIES PUBLIC_HEADER "src/public/include/dvl_gfx_endian.hpp")

add_library(clx_decode INTERFACE)
target_include_directories(clx_decode INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/public/include>
  $<INSTALL_INTERFACE:include>)

add_library(
  clx_encode OBJECT
  src/internal/clx_encode.hpp
  src/internal/clx_encode.cpp)
target_include_directories(clx_encode PRIVATE src/internal)

add_library(
  cel2clx
  src/internal/cel2clx.cpp
)
add_library(DvlGfx::cel2clx ALIAS cel2clx)
target_link_libraries(cel2clx PUBLIC common)
target_link_libraries(cel2clx PRIVATE $<TARGET_OBJECTS:clx_encode>)
set_target_properties(cel2clx PROPERTIES PUBLIC_HEADER "src/public/include/cel2clx.hpp")
target_include_directories(cel2clx PRIVATE src/internal)

add_executable(cel2clx_main src/internal/cel2clx_main.cpp)
set_property(TARGET cel2clx_main PROPERTY RUNTIME_OUTPUT_NAME cel2clx)
target_link_libraries(cel2clx_main PRIVATE cel2clx)
target_include_directories(cel2clx_main PRIVATE src/internal)

add_library(
  cl22clx
  src/internal/cl22clx.cpp
)
add_library(DvlGfx::cl22clx ALIAS cl22clx)
target_link_libraries(cl22clx PUBLIC common)
set_target_properties(cl22clx PROPERTIES PUBLIC_HEADER "src/public/include/cl22clx.hpp")
target_include_directories(cl22clx PRIVATE src/internal)

add_executable(cl22clx_main src/internal/cl22clx_main.cpp)
set_property(TARGET cl22clx_main PROPERTY RUNTIME_OUTPUT_NAME cl22clx)
target_link_libraries(cl22clx_main PRIVATE cl22clx)
target_include_directories(cl22clx_main PRIVATE src/internal)

add_library(
  pcx2clx
  src/internal/pcx.cpp
  src/internal/pcx2clx.cpp)
add_library(DvlGfx::pcx2clx ALIAS pcx2clx)
target_link_libraries(pcx2clx PUBLIC common)
target_link_libraries(pcx2clx PRIVATE $<TARGET_OBJECTS:clx_encode>)
set_target_properties(pcx2clx PROPERTIES PUBLIC_HEADER "src/public/include/pcx2clx.hpp")
target_include_directories(pcx2clx PRIVATE src/internal)

add_executable(pcx2clx_main src/internal/pcx2clx_main.cpp)
set_property(TARGET pcx2clx_main PROPERTY RUNTIME_OUTPUT_NAME pcx2clx)
target_link_libraries(pcx2clx_main PRIVATE pcx2clx)
target_include_directories(pcx2clx_main PRIVATE src/internal)

add_library(
  pixels2clx
  src/internal/pixels2clx.cpp)
add_library(DvlGfx::pixels2clx ALIAS pixels2clx)
target_link_libraries(pixels2clx PUBLIC common)
target_link_libraries(pixels2clx PRIVATE $<TARGET_OBJECTS:clx_encode>)
set_target_properties(pixels2clx PROPERTIES PUBLIC_HEADER "src/public/include/pixels2clx.hpp")
target_include_directories(pixels2clx PRIVATE src/internal)

add_library(
  clx2pixels
  src/internal/clx2pixels.cpp)
add_library(DvlGfx::clx2pixels ALIAS clx2pixels)
target_link_libraries(clx2pixels PUBLIC common clx_decode)
set_target_properties(clx2pixels PROPERTIES PUBLIC_HEADER "src/public/include/clx2pixels.hpp")
target_include_directories(clx2pixels PRIVATE src/internal)

foreach(_target cel2clx_main cl22clx_main pcx2clx_main)
  if(ASAN)
    target_compile_options(${_target} PUBLIC -fsanitize=address -fsanitize-recover=address)
    target_link_libraries(${_target} PUBLIC -fsanitize=address -fsanitize-recover=address)
  endif()

  if(UBSAN)
    target_compile_options(${_target} PUBLIC -fsanitize=undefined)
    target_link_libraries(${_target} PUBLIC -fsanitize=undefined)
  endif()
endforeach()

if(ENABLE_INSTALL)
  install(
    TARGETS cel2clx cl22clx pcx2clx pixels2clx clx2pixels clx_decode common EXPORT DvlGfxTargets
    CONFIGURATIONS Release
    COMPONENT Development
  )
  install(
    TARGETS cel2clx_main cl22clx_main pcx2clx_main
    CONFIGURATIONS Release
    COMPONENT Binaries
  )

  if(NOT DEFINED LIB_INSTALL_DIR)
    set(LIB_INSTALL_DIR lib)
  endif()
  set(ConfigPackageLocation "${LIB_INSTALL_DIR}/cmake/DvlGfx")

  include(CMakePackageConfigHelpers)
  configure_package_config_file(cmake/DvlGfx.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/DvlGfx/DvlGfxConfig.cmake"
    INSTALL_DESTINATION ${ConfigPackageLocation})
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/DvlGfx/DvlGfxConfigVersion.cmake"
    VERSION ${DvlGfx_VERSION}
    COMPATIBILITY AnyNewerVersion
  )

  export(EXPORT DvlGfxTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/DvlGfx/DvlGfxTargets.cmake"
    NAMESPACE DvlGfx::
  )

  install(EXPORT DvlGfxTargets
    FILE
      DvlGfxTargets.cmake
    NAMESPACE
      DvlGfx::
    DESTINATION
      ${ConfigPackageLocation}
    COMPONENT
      Development
  )
  install(
    FILES
      "${CMAKE_CURRENT_BINARY_DIR}/DvlGfx/DvlGfxConfig.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/DvlGfx/DvlGfxConfigVersion.cmake"
    DESTINATION
      ${ConfigPackageLocation}
    COMPONENT
      Development
  )
endif()
