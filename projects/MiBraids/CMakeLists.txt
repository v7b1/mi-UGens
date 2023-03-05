
set(project_name "MiBraids")
project (${project_name})

# this doesn't seem to work... libsamplerate will not be compiled
# add libsamplerate as a "sub-project"
#option(LIBSAMPLERATE_EXAMPLES "libsamplerate: build examples" OFF) 
#option(BUILD_TESTING "libsamplerate: run testing" OFF)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libsamplerate ${CMAKE_CURRENT_SOURCE_DIR}/libsamplerate/build)


# paths to our sources
set(MUTABLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../eurorack")
set(STMLIB_PATH ${MUTABLE_PATH}/stmlib)
set(MI_PATH ${MUTABLE_PATH}/braids)

set(LIBSR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libsamplerate/build/src")
set(LIBSRI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libsamplerate/include")

#set(LIBSR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../build/projects/MiBraids/libsamplerate/src")

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
	${MI_PATH}/settings.h
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


if(APPLE)
# create groups in our project
source_group(TREE ${MUTABLE_PATH} FILES ${STMLIB_SOURCES} ${MI_SOURCES})

target_link_libraries(${PROJECT_NAME} PUBLIC "-framework Accelerate")
endif()

if(MSVC)
        target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBSR_PATH}/Release/samplerate.lib)
else()
        target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBSR_PATH}/libsamplerate.a)
endif()


install(TARGETS ${project_name} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})


