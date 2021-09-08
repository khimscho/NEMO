/*!\file MetadataManager.cpp
 * \brief Metadata element storage and retrieval
 *
 * Data files being uploaded to DCDB need to have metadata elements added in the GeoJSON format
 * for things like name, identification, length, instruments, etc.  Instead of adding these in
 * the cloud (and needing a lookup table), this module allows the data to be stored in the logger
 * instead, where it can be added to each data file and pulled out in the cloud at the appropriate
 * moment to be added to the outgoing file. * Copyright (c) 2021, University of New Hampshire, Center for Coastal and Ocean Mapping.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdint.h>
#include "Arduino.h"
#include "SPIFFS.h"
#include "serialisation.h"
#include "MetadataManager.h"
#include "LogManager.h"

namespace logger {

MetadataManager::MetadataManager(void)
{
    m_backingFile = "/Metadata.txt";
}

MetadataManager::~MetadataManager(void)
{
}

void MetadataManager::WriteMetadata(String const& meta)
{
    File f = SPIFFS.open(m_backingFile, FILE_WRITE);
    f.print(meta);
    f.close();
}

String MetadataManager::GetMetadata(void)
{
    File f = SPIFFS.open(m_backingFile, FILE_READ);
    String meta = f.readString();
    f.close();
    return meta;
}

void MetadataManager::SerialiseMetadata(Serialiser *ser)
{
    Serialisable packet;
    String meta = GetMetadata();

    packet += meta.length();
    packet += meta.c_str();
    ser->Process(logger::Manager::PacketIDs::Pkt_JSON, packet);
}

}