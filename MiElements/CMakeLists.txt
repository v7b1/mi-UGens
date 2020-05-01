cmake_minimum_required (VERSION 2.8)
project (MiElements)

# paths to our sources
set(MUTABLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../eurorack")
set(STMLIB_PATH ${MUTABLE_PATH}/stmlib)
set(MI_PATH ${MUTABLE_PATH}/elements)

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
)

set(MI_SOURCES
        ${MI_PATH}/dsp/dsp.h
        ${MI_PATH}/dsp/exciter.cc
	${MI_PATH}/dsp/exciter.h
	${MI_PATH}/dsp/fx/diffuser.h
	${MI_PATH}/dsp/fx/fx_engine.h
	${MI_PATH}/dsp/fx/reverb.h
	${MI_PATH}/dsp/multistage_envelope.cc
	${MI_PATH}/dsp/multistage_envelope.h
	${MI_PATH}/dsp/ominous_voice.cc
	${MI_PATH}/dsp/ominous_voice.h
	${MI_PATH}/dsp/part.cc
	${MI_PATH}/dsp/part.h
	${MI_PATH}/dsp/patch.h
	${MI_PATH}/dsp/resonator.cc
	${MI_PATH}/dsp/resonator.h
	${MI_PATH}/dsp/string.cc
	${MI_PATH}/dsp/string.h
	${MI_PATH}/dsp/tube.cc
	${MI_PATH}/dsp/tube.h
	${MI_PATH}/dsp/voice.cc
	${MI_PATH}/dsp/voice.h
	${MI_PATH}/resources.cc
	${MI_PATH}/resources.h
)


set(BUILD_SOURCES 
	MiElements.cpp
)



set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
        set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(MiElements MODULE
        ${STMLIB_SOURCES}
        ${MI_SOURCES}
        ${BUILD_SOURCES}
)

# create groups in our project
source_group(TREE ${MUTABLE_PATH} FILES ${STMLIB_SOURCES} ${MI_SOURCES})


# if(APPLE)
# target_link_libraries(${PROJECT_NAME} PUBLIC "-framework Accelerate")
# endif()
