cmake_minimum_required (VERSION 3.8)

set(project_name "MiGrids")
project (${project_name})

# paths to our sources
set(MUTABLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../eurorack")
set(AVRLIB_PATH ${MUTABLE_PATH}/avrlib)
set(MI_PATH ${MUTABLE_PATH}/grids)

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)

include_directories(${MUTABLE_PATH})


set(AVRLIB_SOURCES
        ${AVRLIB_PATH}/base.h
        ${AVRLIB_PATH}/random.cc
        ${AVRLIB_PATH}/random.h
        ${AVRLIB_PATH}/op.h
)

set(MI_SOURCES
        ${MI_PATH}/clock.cc
	${MI_PATH}/clock.h
	${MI_PATH}/pattern_generator.cc
	${MI_PATH}/pattern_generator.h
	${MI_PATH}/resources.cc	
	${MI_PATH}/resources.h
)


set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
        set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(MiGrids MODULE
	${AVRLIB_SOURCES}
        ${MI_SOURCES}
        MiGrids.cpp
)

if(APPLE)
# create groups in our project
source_group(TREE ${MUTABLE_PATH} FILES ${AVRLIB_SOURCES} ${MI_SOURCES})
endif()

install(TARGETS ${project_name} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})
