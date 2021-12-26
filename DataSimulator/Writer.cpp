/*!\file Writer.cpp
 * \brief Manage writing to disc of serialisable packets, with identification numbers
 *
 * This code implements the functionality to generate data on disc in the format associated with
 * the low-cost data logger.  This is a cut-down version of the firmware's log manager, since we
 * only need the bit that writes to disc, rather than management of multiple log files, individual
 * naming, etc.
 *
 */
/// Copyright 2020 Center for Coastal and Ocean Mapping & NOAA-UNH Joint
/// Hydrographic Center, University of New Hampshire.
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights to use,
/// copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
/// and to permit persons to whom the Software is furnished to do so, subject to
/// the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
/// OR OTHER DEALINGS IN THE SOFTWARE.

#include <string>
#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include "Writer.h"

namespace nmea { namespace logger {

/// Constructor for a simple WIBL format output serialiser
///
/// \param filename Local file on which to write the output packets

Writer::Writer(std::string const& filename)
{
    m_outputLog = fopen(filename.c_str(), "wb");
    m_serialiser = new Serialiser(m_outputLog);
    if (m_outputLog == nullptr) {
        std::cout << "error: failed to open output file.\n";
    }
}

/// Destructor for the WIBL-format output serialiser (mostly to manage file resources)

Writer::~Writer(void)
{
    delete m_serialiser;
    if (m_outputLog)
        fclose(m_outputLog);
}

/// Record a packet into the current output file, and check on size (making a new file if
/// required).
///
/// \param pktID    Reference number to save with the packet
/// \param data Serialisable or derived object with data to write

void Writer::Record(PacketIDs pktID, Serialisable const& data)
{
    m_serialiser->Process((uint32_t)pktID, data);
}

}
}
