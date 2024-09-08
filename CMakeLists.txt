cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project(elf-loader)

include(cmake/CPM.cmake)

# Configure cxx version
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add common cpm packages
CpmAddPackage("gh:fmtlib/fmt#11.0.2")
CpmAddPackage("gh:serge1/ELFIO#Release_3.12")

# Create executable
add_executable(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Link libs
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt elfio::elfio)

# Add sources
add_subdirectory(src)
add_subdirectory(test-elf)

# Set binaries and libs dirs for project
set_target_properties (${PROJECT_NAME} test-elf
                       PROPERTIES
                       ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
                       LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
                       RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

