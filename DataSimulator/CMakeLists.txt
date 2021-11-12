# \file CMakeLists.txt
# \brief Make the executable for a simple software data simulator that generates WIBL binary files.
#
# This generates a single executable that allows simulated data to be generated in software on the host
# machine, rather than relying on the embedded processor running the hardware simulator code.  As such, it
# generates bogus data, and it's not hardware accurate, but this may be all that's required for testing.
#
# Copyright (c) 2021, University of New Hampshire, Center for Coastal and Ocean Mapping and
# NOAA/UNH Joint Hydrographic Center.  All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software
# and associated documentation files (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge, publish, distribute,
# sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
# BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(SimulateData)
set(SIM_VERSION_MAJOR 1)
set(SIM_VERSION_MINOR 0)
set(SIM_VERSION_PATCH 0)

if(APPLE)
    # Note warnings being turned off here for CLang and Boost interactions that spawn hundreds of warnings from Boost headers
    add_definitions("-Wno-unused-local-typedefs")
    add_definitions("-Wno-error=unused-local-typedefs")
	# Enforce C++11 for the compiler
    add_definitions("-std=c++11")
endif()

set (SIM_SRC
	Simulator.cpp
	Writer.cpp
	serialisation.cpp
	simulate_data.cpp)

set (SIM_HDR
	Simulator.h
	Writer.h
	serialisation.h)

add_executable(simulate_data ${SIM_SRC} ${SIM_HDR})
target_link_libraries(simulate_data ${LIBS})

install(TARGETS simulate_data RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/bin)