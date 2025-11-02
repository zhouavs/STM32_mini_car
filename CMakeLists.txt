cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME self_demo)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# 自定义 src 目录变量
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# 自动收集 src 目录下所有 C 文件
file(GLOB_RECURSE ALL_C_SOURCES "${SRC_DIR}/*.c")

# 添加到目标
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${ALL_C_SOURCES}
)

# 自动递归查找 src 目录下所有文件夹
file(GLOB_RECURSE ALL_INCLUDE_DIRS LIST_DIRECTORIES true "${SRC_DIR}/*")

# 只保留文件夹路径（去重）
list(REMOVE_DUPLICATES ALL_INCLUDE_DIRS)

# 添加所有文件夹为 include 路径
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${ALL_INCLUDE_DIRS}
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
