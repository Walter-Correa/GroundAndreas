cmake_minimum_required(VERSION 3.5...3.28)
set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING "Minimum policy version" FORCE)
project(GroundAndreas VERSION 1.0.0 LANGUAGES C CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 17)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
  if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -m32")
  endif()
  if(CMAKE_COMPILER_IS_GNUCC
     AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.0)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-attributes")
  elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-ignored-attributes")
  endif()
endif()
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wno-noexcept-type -pthread")
  if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
  endif()
  if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -m32")
  endif()
  if(CMAKE_COMPILER_IS_GNUCXX
     AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.0)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-attributes")
  endif()
endif()

include(CheckIncludeFile)

check_include_file(alloca.h HAVE_ALLOCA_H)
if(HAVE_ALLOCA_H)
  add_definitions(-DHAVE_ALLOCA_H)
endif()
check_include_file(inttypes.h HAVE_INTTYPES_H)
if(HAVE_INTTYPES_H)
  add_definitions(-DHAVE_INTTYPES_H)
endif()
check_include_file(malloc.h HAVE_MALLOC_H)
if(HAVE_MALLOC_H)
  add_definitions(-DHAVE_MALLOC_H)
endif()
check_include_file(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
  add_definitions(-DHAVE_STDINT_H)
endif()

include_directories(
  ./lib/sdk
  ./lib/sdk/amx
)

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0601
                  -D_CRT_SECURE_NO_WARNINGS
                  -DWIN32_LEAN_AND_MEAN)
else()
  add_definitions(-DLINUX)
endif()

set(GROUNDANDREAS_COMMON_SOURCES
  src/heightmap.cpp
  src/natives.cpp
  src/water.cpp
  lib/sdk/amxplugin.cpp
  lib/sdk/amxplugin2.cpp
)

# Traditional SA-MP Plugin Target
set(GROUNDANDREAS_PLUGIN_SOURCES ${GROUNDANDREAS_COMMON_SOURCES} src/main.cpp)
if(WIN32)
  list(APPEND GROUNDANDREAS_PLUGIN_SOURCES src/windows-samp-plugin.def)
endif()

add_library(GroundAndreas MODULE ${GROUNDANDREAS_PLUGIN_SOURCES})

set_target_properties(GroundAndreas PROPERTIES PREFIX "")
set_target_properties(GroundAndreas PROPERTIES OUTPUT_NAME "GroundAndreas")
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(MINGW)
  set_target_properties(GroundAndreas PROPERTIES LINK_FLAGS "-static")
endif()
if(UNIX)
  target_link_libraries(GroundAndreas PRIVATE pthread rt)
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
  set_property(TARGET GroundAndreas APPEND_STRING PROPERTY COMPILE_FLAGS " -Wall")
endif()

# Native open.mp Component Target
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lib/open.mp-sdk/CMakeLists.txt")
  add_subdirectory(lib/open.mp-sdk)
  add_library(GroundAndreas-Component MODULE ${GROUNDANDREAS_COMMON_SOURCES} src/component.cpp)
  target_compile_definitions(GroundAndreas-Component PRIVATE OMP_COMPONENT)
  target_link_libraries(GroundAndreas-Component PRIVATE OMP-SDK)
  if(WIN32)
    target_link_libraries(GroundAndreas-Component PRIVATE ws2_32)
  else()
    target_link_libraries(GroundAndreas-Component PRIVATE pthread rt)
  endif()
  set_target_properties(GroundAndreas-Component PROPERTIES PREFIX "")
  set_target_properties(GroundAndreas-Component PROPERTIES OUTPUT_NAME "GroundAndreas")
  set_target_properties(GroundAndreas-Component PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/components")
  set_target_properties(GroundAndreas-Component PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/components")
  if(MINGW)
    set_target_properties(GroundAndreas-Component PROPERTIES LINK_FLAGS "-static")
  endif()
  if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set_property(TARGET GroundAndreas-Component APPEND_STRING PROPERTY COMPILE_FLAGS " -Wall")
  endif()
endif()
