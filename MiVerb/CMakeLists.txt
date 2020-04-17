cmake_minimum_required (VERSION 3.0)
project (MiVerb)

# paths to our sources
set(SC_PATH /Users/vb/Documents/coding/GitHub/sc-dev/xx_sc_sources/supercollider)
set(MUTABLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../eurorack")
set(STMLIB_PATH ${MUTABLE_PATH}/stmlib)
set(MI_PATH ${MUTABLE_PATH}/rings)

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)

include_directories(${MUTABLE_PATH})


set(STMLIB_SOURCES 
	${STMLIB_PATH}/stmlib.h
	${STMLIB_PATH}/dsp/dsp.h
	${STMLIB_PATH}/dsp/cosine_oscillator.h
	${STMLIB_PATH}/dsp/filter.h
)

set(MI_SOURCES
	# ${MI_PATH}/dsp/dsp.h # do we need this at all??
	${MI_PATH}/dsp/fx/reverb.h
	${MI_PATH}/dsp/fx/fx_engine.h
)

set(BUILD_SOURCES 
	MiVerb.cpp
	#reverb.h
	#fx_engine.h
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

# create groups in our project
source_group(TREE ${MUTABLE_PATH} FILES ${STMLIB_SOURCES} ${MI_SOURCES})
