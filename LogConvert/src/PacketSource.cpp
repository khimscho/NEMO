/*! \file PacketSource.cpp
 *  \brief Implementation of the PacketSource's interfaces
 *
 *  Although PacketSource is generally an abstract interface for sources of NMEA2000 data
 *  that need decoding, it does provide some internal routined to support the process, which
 *  are implemented here.
 *
 * Copyright (c) 2021, University of New Hampshire, Center for Coastal and Ocean Mapping and
 * NOAA/UNH Joint Hydrographic Center.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "PacketSource.h"

/// Default definition of the NextPacket() method for NMEA2000 messages.  Note that if this is not
/// re-implemented by the implementing sub-class, a NotImplemented() exception is thrown.
///
/// \param msg  Reference for where to store the next packet
/// \return True if the packet read was successful; otherwise false.

bool PacketSource::NextPacket(tN2kMsg& msg)
{
    throw NotImplemented();
}

/// Default definition of the NextPacket() method for NMEA0183 messages.  Note that if this is not
/// re-implemented by the implementing sub-class, a NotImplemented() exception is thrown.
///
/// \param elapsed_time Reference for where to store the elapsed time when the sentence was received
/// \param sentence     The NMEA0183 sentence read from the packet source
/// \return True if the packet read was successful; otherwise false.

bool PacketSource::NextPacket(uint32_t& elapsed_time, std::string& sentence)
{
    throw NotImplemented();
}
