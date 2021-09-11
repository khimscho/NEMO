/*! \file NVMFile.cpp
 *  \brief Generic Non-voltile Memory file storage for string data
 *
 * The logger has a number of instances where it needs to store one or more strings in
 * persistent, non-volatile, storage (typically flash memory of some kind).  This module
 * provides a primitive for this, and specialisations for particular cases.
 *
 * Copyright (c) 2021, University of New Hampshire, Center for Coastal and Ocean Mapping.
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

#include "NVMFile.h"
#include "SPIFFS.h"
#include "LogManager.h"
#include "serialisation.h"

namespace logger {

/// \class NVMFile
/// \brief Implementatin of code to read/write a non-volatile file

class NVMFile {
public:
    enum FileMode {
        READ,
        WRITE,
        APPEND
    };

    NVMFile(String const& filename, FileMode mode)
    : m_mode(mode)
    {
        m_source = new File(SPIFFS.open(filename.c_str(), mode == READ ? "r" : mode == WRITE ? "w" : "a"));
    }

    ~NVMFile(void)
    {
        m_source->close();
        delete m_source;
    }

    String ReadNext(void)
    {
        if (m_mode != READ)
            return String("");
        
        return m_source->readString();
    }

    void WriteNext(String const& s)
    {
        if (m_mode == READ)
            return;

        m_source->println(s);
    }

    int Available(void)
    {
        return m_source->available();
    }

private:
    File        *m_source;
    FileMode    m_mode;
};

NVMFileReader::NVMFileReader(String const& filename)
{
    m_source = new NVMFile(filename, NVMFile::FileMode::READ);
}

NVMFileReader::~NVMFileReader(void)
{
    delete m_source;
}

bool NVMFileReader::HasMore(void)
{
    if (m_source->Available())
        return true;
    else
        return false;
}

String NVMFileReader::NextEntry(void)
{
    return m_source->ReadNext();
}

NVMFileWriter::NVMFileWriter(String const& filename, bool append)
{
    NVMFile::FileMode mode;
    if (append) {
        mode = NVMFile::FileMode::APPEND;
    } else {
        mode = NVMFile::FileMode::WRITE;
    }
    m_source = new NVMFile(filename, mode);
}

NVMFileWriter::~NVMFileWriter(void)
{
    delete m_source;
}

void NVMFileWriter::AddEntry(String const& s)
{
    m_source->WriteNext(s);
}

MetadataStore::MetadataStore(void)
{
    m_backingStore = String("/Metadata.txt");
}

void MetadataStore::WriteMetadata(String const& s)
{
    NVMFileWriter w(m_backingStore, false);
    w.AddEntry(s);
}

String MetadataStore::GetMetadata(void)
{
    NVMFileReader r(m_backingStore);
    return r.NextEntry();
}

void MetadataStore::SerialiseMetadata(Serialiser *s)
{
    String meta = GetMetadata();
    Serialisable packet;

    packet += meta.length();
    packet += meta.c_str();
    s->Process(logger::Manager::PacketIDs::Pkt_JSON, packet);
}

AlgoRequestStore::AlgoRequestStore(void)
{
    m_algoBackingStore = "/algorithms.txt";
    m_paramBackingStore = "/parameters.txt";
}

void AlgoRequestStore::AddAlgorithm(String const& alg_name, String const& alg_params, bool restart)
{
    NVMFileWriter alg(m_algoBackingStore, ~restart);
    NVMFileWriter par(m_paramBackingStore, ~restart);
    alg.AddEntry(alg_name);
    par.AddEntry(alg_params);
}

void AlgoRequestStore::ListAlgorithms(Stream& s)
{
    NVMFileReader alg(m_algoBackingStore);
    NVMFileReader par(m_paramBackingStore);

    while (alg.HasMore()) {
        s.printf("alg = \"%s\", params = \"%s\"\n", alg.NextEntry().c_str(), par.NextEntry().c_str());
    }
}

void AlgoRequestStore::SerialiseAlgorithms(Serialiser *s)
{
    NVMFileReader alg(m_algoBackingStore);
    NVMFileReader par(m_paramBackingStore);
    
    while (alg.HasMore()) {
        Serialisable ser(255);
        String algorithm = alg.NextEntry();
        String parameters = par.NextEntry();
        ser += algorithm.length();
        ser += algorithm.c_str();
        ser += parameters.length();
        ser += parameters.c_str();
        s->Process(logger::Manager::PacketIDs::Pkt_Algorithms, ser);
    }
}

N0183IDStore::N0183IDStore(void)
{
    m_backingStore = "/NMEA0183IDs.txt";
}

void N0183IDStore::AddID(String const& msgid, bool restart)
{
    NVMFileWriter w(m_backingStore, ~restart);
    w.AddEntry(msgid);
}

void N0183IDStore::ListIDs(Stream& s)
{
    NVMFileReader r(m_backingStore);
    while (r.HasMore()) {
        s.println(r.NextEntry());
    }
}

void N0183IDStore::SerialiseIDs(Serialiser *s)
{
    NVMFileReader r(m_backingStore);

    while (r.HasMore()) {
        Serialisable ser;
        String IDname = r.NextEntry();
        ser += IDname.length();
        ser += IDname.c_str();
        s->Process(logger::Manager::PacketIDs::Pkt_NMEA0183ID, ser);
    }
}

}