/*!\file Writer.cpp
 * \brief Manage writing to disc of serialisable packets, with identification numbers
 *
 * This code implements the functionality to generate data on disc in the format associated with
 * the low-cost data logger.  This is a cut-down version of the firmware's log manager, since we
 * only need the bit that writes to disc, rather than management of multiple log files, individual
 * naming, etc.
 *
 * Copyright 2020, University of New Hampshire, Center for Coastal and Ocean Mapping and
 * NOAA/UNH Joint Hydrographic Center.  All Rights Reserved.
 */

#include <string>
#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include "Writer.h"

namespace nmea { namespace logger {

Writer::Writer(std::string const& filename)
{
    m_outputLog = fopen(filename.c_str(), "wb");
    m_serialiser = new Serialiser(m_outputLog);
    if (m_outputLog == nullptr) {
        std::cout << "error: failed to open output file.\n";
    }
}

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
