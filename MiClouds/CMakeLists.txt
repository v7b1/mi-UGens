cmake_minimum_required (VERSION 3.0)
project (MiClouds)

# paths to our sources
set(MUTABLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../eurorack")
set(STMLIB_PATH ${MUTABLE_PATH}/stmlib)
set(MI_PATH ${MUTABLE_PATH}/clouds)

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)

include_directories(${MUTABLE_PATH})


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
	${STMLIB_PATH}/fft/shy_fft.h
)

set(MI_SOURCES
	${MI_PATH}/dsp/audio_buffer.h
	${MI_PATH}/dsp/correlator.cc
	${MI_PATH}/dsp/correlator.h
	${MI_PATH}/dsp/frame.h
	${MI_PATH}/dsp/fx/diffuser.h
	${MI_PATH}/dsp/fx/fx_engine.h
	${MI_PATH}/dsp/fx/pitch_shifter.h
	${MI_PATH}/dsp/fx/reverb.h
	${MI_PATH}/dsp/grain.h
	${MI_PATH}/dsp/granular_processor.cc
	${MI_PATH}/dsp/granular_processor.h
	${MI_PATH}/dsp/granular_sample_player.h
	${MI_PATH}/dsp/looping_sample_player.h
	${MI_PATH}/dsp/mu_law.cc
	${MI_PATH}/dsp/mu_law.h
	${MI_PATH}/dsp/parameters.h

	${MI_PATH}/dsp/pvoc/frame_transformation.cc
	${MI_PATH}/dsp/pvoc/frame_transformation.h
	${MI_PATH}/dsp/pvoc/phase_vocoder.cc
	${MI_PATH}/dsp/pvoc/phase_vocoder.h
	${MI_PATH}/dsp/pvoc/stft.cc
	${MI_PATH}/dsp/pvoc/stft.h

	${MI_PATH}/dsp/sample_rate_converter.h
	${MI_PATH}/dsp/window.h
	${MI_PATH}/dsp/wsola_sample_player.h

	${MI_PATH}/resources.cc
	${MI_PATH}/resources.h
)

set(BUILD_SOURCES 
	MiClouds.cpp
)


set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(
	MiClouds MODULE
    ${STMLIB_SOURCES}
    ${MI_SOURCES}
    ${BUILD_SOURCES}
)

if(APPLE)
# create groups in our project
source_group(TREE ${MUTABLE_PATH} FILES ${STMLIB_SOURCES} ${MI_SOURCES})
endif()
