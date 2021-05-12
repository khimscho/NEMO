/*! \file TeamSurvSource.cpp
 *  \brief Implementation of PacketSource for the TeamSurv "TSV" format.
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

#include "TeamSurvSource.h"

TeamSurvSource::TeamSurvSource(FILE *in)
: PacketSource(), m_file(in)
{
    m_buffer = new char[1024];
    m_bufferLen = 1024;
}

TeamSurvSource::~TeamSurvSource(void)
{
    delete m_buffer;
}

bool TeamSurvSource::NextPacket(uint32_t& elapsed_time, std::string& sentence)
{
    uint32_t len;
    bool complete = false;
    do {
        if (fgets(m_buffer, m_bufferLen, m_file) == nullptr) {
            if (feof(m_file) != 0)
                return false;
        }
        len = strlen(m_buffer);
        if (len > 11) {
            // For a plausible sentence, we need to have at least the leading "$TTSSS" string,
            // and a "*XX" checksum.  We also remove the \r\n that all strings appear to have.
            m_buffer[len-2] = '\0';
            len -= 2;
            if (m_buffer[0] == '$' && m_buffer[len-3] == '*') {
                int chk = 0;
                for (int i = 1; i < len-3; ++i) {
                    chk ^= m_buffer[i];
                }
                char checksum[3];
                sprintf(checksum, "%02X", chk);
                if (m_buffer[len-2] == checksum[0] && m_buffer[len-1] == checksum[1])
                    complete = true;
            }
        }
    } while (!complete);
    
    elapsed_time = 0; // TeamSurv does not provide timestamps ...
    sentence = std::string(m_buffer);
    return true;
}
