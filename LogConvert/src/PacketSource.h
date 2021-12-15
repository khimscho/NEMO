/*! \file PacketSource.h
 *  \brief Definition of the PacketSource's interfaces
 *
 *  PacketSource provides a (mostly) abstract interface for any source of data packets of
 *  raw NMEA2000 data that need decoding.
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

#ifndef __PACKET_SOURCE_H__
#define __PACKET_SOURCE_H__

#include <string>
#include <stdint.h>
#include "N2kMsg.h"

/// \class NotImplemented
/// \brief Exception class to indicate that the virtual methods required have not been re-implemented
///
/// The NextPacket() method of \a PacketSource can be defined for NMEA packets from NMEA2000 or NMEA0183,
/// and therefore is unlikely to be defined in both cases for any given source.  This exception is thrown
/// if the wrong method is called, to provide development support.
class NotImplemented {};

/// \class PacketSource
/// \brief Base class for any source of raw NMEA packets to be decoded

class PacketSource {
public:
    PacketSource(void) {}
    virtual ~PacketSource(void) {}
    
    /// \brief Extract the next packet from the source (for NMEA2000 packets)
    virtual bool NextPacket(tN2kMsg& msg);

    /// \brief Extract the next packet from the source (for NMEA0183 packets)
    virtual bool NextPacket(uint32_t& elapsed_time, std::string& sentence);
    
    /// \brief Define whether the packet source is NMEA2000 or NMEA0183
    ///
    /// Since we can read packets from either type of NMEA source, but the signature
    /// of the reader method is different, this provides a mechanism to determine which
    /// method to use.
    ///
    /// \return True if the source is NMEA2000, otherwise false.
    virtual bool IsN2k(void) = 0;
};

#endif
