cmake_minimum_required (VERSION 2.8)
project (MiMu)

# paths to our sources
# set(SC_PATH /Users/vb/Documents/coding/sc-dev/supercollider-3.10)
# path should be specified by user!

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)



set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
        set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(MiMu MODULE
        MiMu.cpp
)