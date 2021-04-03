cmake_minimum_required (VERSION 3.8)
project (MiOmi)

# paths to our sources
set(MUTABLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../eurorack")
set(STMLIB_PATH ${MUTABLE_PATH}/stmlib)
set(MI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/mi")

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)

include_directories(
	${MUTABLE_PATH}
	${MI_PATH}
)

if(MSVC)
    add_definitions(-D_USE_MATH_DEFINES) # defines M_PI with MSVC
endif()

set(MI_SOURCES
    ${MI_PATH}/omi/dsp/dsp.h
    ${MI_PATH}/omi/dsp/part.cc
    ${MI_PATH}/omi/dsp/part.h
    ${MI_PATH}/omi/dsp/patch.h
    ${MI_PATH}/omi/dsp/ominous_voice.cc
    ${MI_PATH}/omi/dsp/ominous_voice.h
    ${MI_PATH}/omi/dsp/multistage_envelope.cc
    ${MI_PATH}/omi/dsp/multistage_envelope.h
    ${MI_PATH}/omi/resources.cc
    ${MI_PATH}/omi/resources.h
)

set(BUILD_SOURCES 
	MiOmi.cpp
)


set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(
	MiOmi MODULE
    ${MI_SOURCES}
    ${BUILD_SOURCES}
)

if(APPLE)
# create groups in our project
source_group(TREE ${MI_PATH} FILES ${MI_SOURCES})
endif()
