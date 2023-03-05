cmake_minimum_required (VERSION 3.8)
project (MiRipples)

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)

set(MUTABLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../mutableSources32")
set(VCV_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcvrack")
set(VCV_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/vcvrack/include")

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(MSVC)
    add_definitions(-D_USE_MATH_DEFINES) # defines M_PI with MSVC
endif()

set(VCV_SOURCES 
	${VCV_PATH}/Ripples/aafilter.hpp
	${VCV_PATH}/Ripples/ripples.hpp
	${VCV_PATH}/Ripples/sos.hpp
	${VCV_PATH}/src/random.cpp
)


set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
        set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(
        MiRipples 
        MODULE
        MiRipples.cpp
        ${VCV_SOURCES}
)

include_directories(
        ${VCV_PATH}
        ${VCV_INCLUDE}
)

if(APPLE)
# create groups in our project
source_group(TREE ${VCV_PATH} FILES ${VCV_SOURCES})
endif()
