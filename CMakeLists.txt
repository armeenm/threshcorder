cmake_minimum_required(VERSION 3.12...3.17 FATAL_ERROR)
project(threshcorder
  VERSION 0.0.1
  DESCRIPTION "Threshold-based recorder"
  LANGUAGES CXX)

option(FORCE_COLORED_OUTPUT
  "Always produce ANSI-colored output (GNU/Clang only)" 
  OFF)

option(STATIC_ANALYSIS
  "Enable static analysis tools"
  OFF)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED SHARED)

set(BUILD_OPTS 
  -Wshadow
  -Wduplicated-cond
  -Wlogical-op
  -Wduplicated-branches
  -Wnull-dereference
  -Wold-style-cast
  -Wuseless-cast
  -Wformat=2 
  -Wall
  -Wextra
  -Wconversion
  -Wpedantic
  -pipe
  -flto
  -fsanitize=undefined)

file(GLOB_RECURSE SRCS
  LIST_DIRECTORIES false
  CONFIGURE_DEPENDS
  "threshcorder/*.cpp")

add_executable(threshcorder ${SRCS})

target_include_directories(threshcorder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(threshcorder PRIVATE ${BUILD_OPTS})
target_compile_features(threshcorder PUBLIC cxx_std_17)

if(${FORCE_COLORED_OUTPUT})
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(threshcorder PRIVATE -fdiagnostics-color=always)
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(threshcorder PRIVATE -fcolor-diagnostics)
  endif()
endif()

if(${STATIC_ANALYSIS})
  set_target_properties(threshcorder PROPERTIES
    CXX_CLANG_TIDY "clang-tidy;-checks=*;-header-filter=.*"
    CXX_CPPCHECK "cppcheck;--std=c++11")
endif()

target_link_libraries(threshcorder PRIVATE
  asound
  ubsan
  fmt::fmt
  spdlog::spdlog)
