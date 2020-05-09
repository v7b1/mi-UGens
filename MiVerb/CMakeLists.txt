cmake_minimum_required (VERSION 3.0)
project (MiVerb)

# paths to our sources
set(MUTABLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../eurorack")
set(STMLIB_PATH ${MUTABLE_PATH}/stmlib)
set(HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include")


include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)

include_directories(${MUTABLE_PATH} ${HEADER_PATH})


set(STMLIB_SOURCES 
	${STMLIB_PATH}/stmlib.h
	${STMLIB_PATH}/dsp/dsp.h
	${STMLIB_PATH}/dsp/cosine_oscillator.h
	${STMLIB_PATH}/dsp/filter.h
)

set(MI_SOURCES
	${HEADER_PATH}/reverb.h
	${HEADER_PATH}/fx_engine.h
)

set(BUILD_SOURCES 
	MiVerb.cpp
)


set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(
	MiVerb MODULE
    ${STMLIB_SOURCES}
    ${MI_SOURCES}
    ${BUILD_SOURCES}
)

if(APPLE)
# create groups in our project
source_group(TREE ${MUTABLE_PATH} FILES ${STMLIB_SOURCES})
endif()
