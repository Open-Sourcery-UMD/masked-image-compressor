cmake_minimum_required(VERSION 3.10)
project(micpp)

include(FetchContent)

# C++23, no extensions
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)

message("Build Type: ${CMAKE_BUILD_TYPE}")

# Enable UBSan and ASan on debug builds
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined")
endif()

# Download stb_image
FetchContent_Declare(
        stb_image
        URL "https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h"
        DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/include/"
        DOWNLOAD_NO_EXTRACT ON
)

# Download stb_image_write
FetchContent_Declare(
        stb_image_write
        URL "https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image_write.h"
        DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/include/"
        DOWNLOAD_NO_EXTRACT ON
)

# Download argparse
FetchContent_Declare(
        argparse
        URL "https://raw.githubusercontent.com/p-ranav/argparse/refs/heads/master/include/argparse/argparse.hpp"
        DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/include/"
        DOWNLOAD_NO_EXTRACT ON
)

FetchContent_MakeAvailable(stb_image stb_image_write argparse)

# Set a universal directory to access downloaded headers from
set(MICPP_DEP_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/include")

# Main target for the compressor
add_executable(micompress src/micompress.cpp)
target_include_directories(micompress PRIVATE include ${MICPP_DEP_INCLUDE})

# dctdump, a tool for dumping the DCT coefficients of a vector
add_executable(dctdump src/dctdump.cpp src/image.cpp)
target_include_directories(dctdump PRIVATE include ${MICPP_DEP_INCLUDE})
