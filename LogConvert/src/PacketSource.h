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

class NotImplemented {};

/// \class PacketSource
/// \brief Base class for any source of raw NMEA2000 packets to be decoded

class PacketSource {
public:
    PacketSource(void) {}
    virtual ~PacketSource(void) {}
    
    virtual bool NextPacket(tN2kMsg& msg);
    virtual bool NextPacket(uint32_t& elapsed_time, std::string& sentence);
    
    virtual bool IsN2k(void) = 0;
};

#endif
