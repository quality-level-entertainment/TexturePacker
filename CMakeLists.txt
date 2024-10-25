# Executable CMake
# make sure to have the main.cpp file within the "src/". Otherwise, the program will not be able to run
cmake_minimum_required(VERSION 3.10)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the project name
project(TexturePacker
    VERSION 1.0
    DESCRIPTION "TexturePacker - A C++20 tool to create spritesheets"
    LANGUAGES CXX)

# Specify the source and include directories
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Recursively collect all source files (.cpp and .h)
# create the library using the collected source files
file(GLOB_RECURSE SOURCES "${SRC_DIR}/*.cpp" "${SRC_DIR}/*.h")

# Retain folder hierarchy by specifying the source files with their full paths
source_group(TREE "${SRC_DIR}"
    PREFIX "Source Files"
    FILES ${SOURCES})

if(WIN32)
    add_executable(${PROJECT_NAME} ${SOURCES} icon.rc)
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

# Add dependencies
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Set the Visual Studio startup project
if (CMAKE_GENERATOR MATCHES "Visual Studio")
set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()