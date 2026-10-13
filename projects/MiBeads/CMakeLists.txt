set(project_name "MiBeads")
project (${project_name})

# paths to our sources
set(MUTABLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../eurorack")
set(STMLIB_PATH ${MUTABLE_PATH}/stmlib)
set(MI_PATH ${MUTABLE_PATH}/beads)

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)

include_directories(${MUTABLE_PATH})

if(MSVC)
	add_definitions(-D_USE_MATH_DEFINES) # defines M_PI with MSVC
endif()

set(STMLIB_SOURCES
	${STMLIB_PATH}/stmlib.h
	${STMLIB_PATH}/utils/random.cc
	${STMLIB_PATH}/utils/random.h
	${STMLIB_PATH}/utils/gate_flags.h
	${STMLIB_PATH}/dsp/units.cc
	${STMLIB_PATH}/dsp/units.h
	${STMLIB_PATH}/dsp/dsp.h
	${STMLIB_PATH}/dsp/hysteresis_filter.h
	${STMLIB_PATH}/dsp/hysteresis_quantizer.h
	${STMLIB_PATH}/dsp/parameter_interpolator.h
)

set(MI_SOURCES
	${MI_PATH}/dsp/delay_clock.cc
	${MI_PATH}/dsp/delay_clock.h
	${MI_PATH}/dsp/delay_controller.cc
	${MI_PATH}/dsp/delay_controller.h
	${MI_PATH}/dsp/delay_line.h
	${MI_PATH}/dsp/frame.h
	${MI_PATH}/dsp/fx/fx_engine.h
	${MI_PATH}/dsp/fx/reverb.h
	${MI_PATH}/dsp/fx/wow_and_flutter.h
	${MI_PATH}/dsp/granular_processor.h
	${MI_PATH}/dsp/mu_law.cc
	${MI_PATH}/dsp/mu_law.h
	${MI_PATH}/dsp/processor.cc
	${MI_PATH}/dsp/processor.h
	${MI_PATH}/dsp/synth.cc
	${MI_PATH}/dsp/synth.h
	${MI_PATH}/dsp/trigger_generator.cc
	${MI_PATH}/dsp/trigger_generator.h

	${MI_PATH}/resources.cc
	${MI_PATH}/resources.h
)

set(BUILD_SOURCES
	MiBeads.cpp
)


set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(
	MiBeads MODULE
	${STMLIB_SOURCES}
	${MI_SOURCES}
	${BUILD_SOURCES}
)

# add preprocessor macro TEST to avoid asm functions
target_compile_definitions(${PROJECT_NAME} PUBLIC TEST)

if(APPLE)
# create groups in our project
source_group(TREE ${MUTABLE_PATH} FILES ${STMLIB_SOURCES} ${MI_SOURCES})
endif()

install(TARGETS ${project_name} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})
