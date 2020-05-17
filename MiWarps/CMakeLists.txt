cmake_minimum_required (VERSION 2.8)
project (MiWarps)

# paths to our sources
set(MUTABLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../eurorack")
set(STMLIB_PATH ${MUTABLE_PATH}/stmlib)
set(MI_PATH ${MUTABLE_PATH}/warps)
# set(LIBSR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libsamplerate/build")
# set(LIBSRI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libsamplerate/src")

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)

include_directories(${MUTABLE_PATH})
include_directories(${LIBSR_PATH})
include_directories(${LIBSRI_PATH})


set(STMLIB_SOURCES
        ${STMLIB_PATH}/stmlib.h
	${STMLIB_PATH}/utils/random.cc
	${STMLIB_PATH}/utils/random.h
	${STMLIB_PATH}/dsp/units.cc
	${STMLIB_PATH}/dsp/units.h
)

set(MI_SOURCES
        ${MI_PATH}/dsp/filter_bank.cc
	${MI_PATH}/dsp/filter_bank.h
	${MI_PATH}/dsp/limiter.h
	${MI_PATH}/dsp/modulator.cc
	${MI_PATH}/dsp/modulator.h
	${MI_PATH}/dsp/oscillator.cc
	${MI_PATH}/dsp/oscillator.h
	${MI_PATH}/dsp/parameters.h
	${MI_PATH}/dsp/quadrature_oscillator.h
	${MI_PATH}/dsp/quadrature_transform.h
	${MI_PATH}/dsp/sample_rate_conversion_filters.h
	${MI_PATH}/dsp/sample_rate_converter.h
	${MI_PATH}/dsp/vocoder.cc
	${MI_PATH}/dsp/vocoder.h
	${MI_PATH}/resources.cc
	${MI_PATH}/resources.h
	${MI_PATH}/settings.cc
	${MI_PATH}/settings.h
)


set(BUILD_SOURCES 
	MiWarps.cpp
)



set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
        set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(MiWarps MODULE
        ${STMLIB_SOURCES}
        ${MI_SOURCES}
        ${BUILD_SOURCES}
)

if(APPLE)
# create groups in our project
source_group(TREE ${MUTABLE_PATH} FILES ${STMLIB_SOURCES} ${MI_SOURCES})

# target_link_libraries(${PROJECT_NAME} PUBLIC "-framework Accelerate")
endif()


# target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBSR_PATH}/libsamplerate.a)

