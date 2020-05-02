cmake_minimum_required (VERSION 2.8)
project (MiBraids)

# paths to our sources
set(MUTABLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../eurorack")
set(STMLIB_PATH ${MUTABLE_PATH}/stmlib)
set(MI_PATH ${MUTABLE_PATH}/braids)
# set(LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../libs")
set(LIBSR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libsamplerate/build")
set(LIBSRI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libsamplerate/src")

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
	${STMLIB_PATH}/utils/dsp.h
	${STMLIB_PATH}/dsp/atan.cc
	${STMLIB_PATH}/dsp/atan.h
	${STMLIB_PATH}/dsp/units.cc
	${STMLIB_PATH}/dsp/units.h
	${STMLIB_PATH}/dsp/dsp.h

)

set(MI_SOURCES 
	${MI_PATH}/analog_oscillator.cc
	${MI_PATH}/analog_oscillator.h
	${MI_PATH}/digital_oscillator.cc
	${MI_PATH}/digital_oscillator.h
	${MI_PATH}/envelope.h
	${MI_PATH}/excitation.h
	${MI_PATH}/macro_oscillator.cc
	${MI_PATH}/macro_oscillator.h
	${MI_PATH}/parameter_interpolation.h
	${MI_PATH}/quantizer.cc
	${MI_PATH}/quantizer.h
	${MI_PATH}/quantizer_scales.h
	${MI_PATH}/resources.cc
	${MI_PATH}/resources.h
	${MI_PATH}/signature_waveshaper.h
	${MI_PATH}/vco_jitter_source.h
)



set(BUILD_SOURCES 
	MiBraids.cpp
)



set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(MiBraids MODULE
        ${STMLIB_SOURCES}
        ${MI_SOURCES}
        ${BUILD_SOURCES}
)

# create groups in our project
source_group(TREE ${MUTABLE_PATH} FILES ${STMLIB_SOURCES} ${MI_SOURCES})

if(APPLE)
target_link_libraries(${PROJECT_NAME} PUBLIC "-framework Accelerate")
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBSR_PATH}/libsamplerate.a)

