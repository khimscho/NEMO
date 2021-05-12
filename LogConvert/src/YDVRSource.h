/*! \file YDVRSource.h
 *  \brief Definition of PacketSource for the Yacht Devices YDVR DAT format.
 *
 *  This provides the concrete implementation of the PacketSource abstract interface
 *  specialised for the Yacht Devices YDVR04's DAT data format (encoded NMEA2000 packets
 *  with internal timestamp and PGN number).
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

#ifndef __YDVR_SOURCE_H__
#define __YDVR_SOURCE_H__

#include <cstdio>
#include "PacketSource.h"

class DataPacketTooLarge {};

/// \class YDVRSource
/// \brief Implementation of PacketSource for YDVR04 DAT format

class YDVRSource : public PacketSource {
public:
    YDVRSource(FILE *f);
    ~YDVRSource(void);
    
    bool NextPacket(tN2kMsg& msg);
    
    bool IsN2k(void) { return true; }
    
private:
    FILE    *m_source;
    
    bool IsMultiPacket(uint32_t pgn);
};

#endif
