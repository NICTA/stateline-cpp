# Copyright (c) 2014, NICTA.
# Lesser General Public License version 3 or later
# See the COPYRIGHT file.

# Authors: Lachlan McCalman, Darren Shen
# Date: 2016

cmake_minimum_required(VERSION 3.1)
project(STATELINE LANGUAGES CXX)

# Enable unit tests
enable_testing()

# CMake modules path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

# Dependencies
find_package(ZMQ REQUIRED)

# Use Modern C++
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)

include_directories("include"
                    SYSTEM "external"
                    SYSTEM "${ZMQ_INCLUDE_DIR}")

add_subdirectory(examples)
add_subdirectory(tests)
