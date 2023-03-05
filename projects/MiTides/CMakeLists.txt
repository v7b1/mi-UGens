cmake_minimum_required (VERSION 3.8)
project (MiTides)

# paths to our sources
set(MUTABLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../eurorack")
set(STMLIB_PATH ${MUTABLE_PATH}/stmlib)
set(MI_PATH ${MUTABLE_PATH}/tides2)

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)

include_directories(${MUTABLE_PATH})


set(STMLIB_SOURCES 
	${STMLIB_PATH}/stmlib.h

	${STMLIB_PATH}/dsp/dsp.h
	${STMLIB_PATH}/dsp/hysteresis_quantizer.h
	${STMLIB_PATH}/dsp/polyblep.h
)

set(MI_SOURCES
	${MI_PATH}/ramp_generator.h
	${MI_PATH}/ramp_shaper.h
	${MI_PATH}/ratio.h
	${MI_PATH}/resources.cc
	${MI_PATH}/resources.h
	${MI_PATH}/poly_slope_generator.cc
	${MI_PATH}/poly_slope_generator.h
	${MI_PATH}/ramp_extractor.cc
	${MI_PATH}/ramp_extractor.h
)

set(BUILD_SOURCES 
	MiTides.cpp
)


set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(
	MiTides MODULE
    ${STMLIB_SOURCES}
    ${MI_SOURCES}
    ${BUILD_SOURCES}
)

# add preprocessor macro TEST
target_compile_definitions(MiTides PUBLIC TEST)

if(APPLE)
# create groups in our project
source_group(TREE ${MUTABLE_PATH} FILES ${STMLIB_SOURCES} ${MI_SOURCES})
endif()
