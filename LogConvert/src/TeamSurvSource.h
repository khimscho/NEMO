/*! \file TeamSurvSource.h
 *  \brief Definition of PacketSource for the TeamSurv "TSV" format.
 *
 *  This provides the concrete implementation of the PacketSource abstract interface
 *  specialised for the TeamSurv data logger (which is essentially just raw NMEA0183 strings
 *  without any timestamps or other decorations).
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

#ifndef __TEAMSURV_SOURCE_H__
#define __TEAMSURV_SOURCE_H__

#include <cstdio>
#include "PacketSource.h"

/// \class TeamSurvSource
/// \brief Simple implementation of PacketSource for raw NMEA0183 strings

class TeamSurvSource : public PacketSource {
public:
    TeamSurvSource(FILE *in);
    ~TeamSurvSource(void);
    
    bool NextPacket(uint32_t& elapsed_time, std::string& sentence);
    
    bool IsN2k(void) { return false; }
    
private:
    FILE        *m_file;
    char        *m_buffer;
    uint32_t    m_bufferLen;
};

#endif
