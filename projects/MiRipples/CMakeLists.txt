

set(project_name "MiRipples")
project (${project_name})

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)

#set(MUTABLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../mutableSources32")
set(VCV_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcvrack")
set(VCV_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/vcvrack/include")



if(MSVC)
    add_definitions(-D_USE_MATH_DEFINES) # defines M_PI with MSVC
	# specify the C++ standard
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_STANDARD_REQUIRED True)
endif()

set(VCV_SOURCES 
	${VCV_PATH}/Ripples/aafilter.hpp
	${VCV_PATH}/Ripples/ripples.hpp
	${VCV_PATH}/Ripples/sos.hpp
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
	if (CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
            set(VCV_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/vcvrack/dep")
    	endif()
)

if(APPLE)
# create groups in our project
source_group(TREE ${VCV_PATH} FILES ${VCV_SOURCES})
endif()


install(TARGETS ${project_name} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})