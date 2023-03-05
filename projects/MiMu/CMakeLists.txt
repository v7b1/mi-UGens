cmake_minimum_required (VERSION 3.8)
project (MiMu)

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