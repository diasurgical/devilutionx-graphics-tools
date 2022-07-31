cmake_minimum_required(VERSION 3.16)

project(DevilutionxTools VERSION 0.0.1 LANGUAGES C CXX)

option(ASAN "Enable address sanitizer" ON)
option(UBSAN "Enable undefined behaviour sanitizer" ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

if(CMAKE_BUILD_TYPE MATCHES "Release")
  set(ASAN OFF)
  set(UBSAN OFF)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clang-tidy

add_subdirectory(third_party/tl)

add_executable(pcx2clx
  src/pcx2clx_main.cpp
  src/pcx2clx.cpp
  src/pcx.cpp
)
add_executable(cel2clx
  src/cel2clx_main.cpp
  src/cel2clx.cpp
)

foreach(_target pcx2clx cel2clx)
  target_include_directories(${_target} PRIVATE src)
  target_link_libraries(${_target} PRIVATE tl)

  if(ASAN)
    target_compile_options(${_target} PUBLIC -fsanitize=address -fsanitize-recover=address)
    target_link_libraries(${_target} PUBLIC -fsanitize=address -fsanitize-recover=address)
  endif()

  if(UBSAN)
    target_compile_options(${_target} PUBLIC -fsanitize=undefined)
    target_link_libraries(${_target} PUBLIC -fsanitize=undefined)
  endif()
endforeach()
