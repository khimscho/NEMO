# \file CMakeLists.txt
# \brief Make the executable for VBI log data conversion
#
# This generates a single executable that allows conversion of a variety of input data
# formats from volunteer bathymetric information loggers into a standard format that can
# then be decoded or further processed.
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

set(DECODER_SRC
	serialisation.cpp
	SerialisableFactory.cpp
	PacketSource.cpp
	YDVRSource.cpp
    TeamSurvSource.cpp
	logconvert.cpp)
		
set(DECODER_HDR
	serialisation.h
	SerialisableFactory.h
	PacketSource.h
	YDVRSource.h
    TeamSurvSource.h)

add_executable(logconvert ${DECODER_SRC} ${DECODER_HDR})
target_link_libraries(logconvert ${LIBS})

install(TARGETS logconvert RUNTIME DESTINATION bin)
