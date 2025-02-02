cmake_minimum_required(VERSION 3.19)
set(lib_name "mi-UGens")
project("${lib_name}")

if (APPLE)
    if (NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architecture" FORCE)
    endif()
endif()


# specify the C++ standard
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17" CACHE STRING "c++ version" FORCE)
#if(CMAKE_COMPILER_IS_CLANG)
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
#endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# set installation directory
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/${lib_name}" CACHE PATH "Install prefix" FORCE)
else()
  set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/${lib_name}")
endif()


message(STATUS "Install directory set to: ${CMAKE_INSTALL_PREFIX}")

# set some options for libsamplerate

option(LIBSAMPLERATE_EXAMPLES "libsamplerate: build examples" OFF) 
option(BUILD_TESTING "libsamplerate: run testing" OFF)

# add_subdirectories manually
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/MiBraids/libsamplerate ${CMAKE_CURRENT_SOURCE_DIR}/projects/MiBraids/libsamplerate/build)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/MiBraids)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/MiClouds)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/MiElements)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/MiGrids)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/MiMu)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/MiOmi)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/MiPlaits)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/MiRings)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/MiRipples)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/MiTides)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/MiVerb)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/projects/MiWarps)



# Install sc classes and help files
install(DIRECTORY "sc/Classes" "sc/HelpSource" DESTINATION "${CMAKE_INSTALL_PREFIX}")