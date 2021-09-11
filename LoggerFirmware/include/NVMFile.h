/*! \file NVMFile.h
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

#ifndef __NVMFILE_H__
#define __NVMFILE_H__

#include "Arduino.h"
#include "serialisation.h"

namespace logger {

// Forward declaration of base class, not required to be defined here
class NVMFile;

/// \class NVMFileReader
/// \brief Access class to read data from a NVMFile

class NVMFileReader {
public:
    NVMFileReader(String const& filename);
    ~NVMFileReader(void);

    bool HasMore(void);
    String NextEntry(void);

private:
    NVMFile *m_source;
};

/// \class NVMWriter
/// \brief Access class to write data to a NVMFile

class NVMFileWriter {
public:
    NVMFileWriter(String const& filename, bool append = true);
    ~NVMFileWriter(void);

    void AddEntry(String const& s);

private:
    NVMFile *m_source;
};

/// \class MetadataStore
/// \brief Specialisation of NVMFile for metadata storage

class MetadataStore {
public:
    MetadataStore(void);

    void WriteMetadata(String const& meta);
    String GetMetadata(void);
    void SerialiseMetadata(Serialiser *ser);

private:
    String  m_backingStore;
};

/// \class AlgoRequestStore
/// \brief Specialisation of NVMFile for algorithm request storage

class AlgoRequestStore {
public:
    AlgoRequestStore(void);

    void AddAlgorithm(String const& alg_name, String const& alg_params, bool restart = false);
    void ListAlgorithms(Stream& s);
    void SerialiseAlgorithms(Serialiser *s);

private:
    String  m_algoBackingStore;
    String  m_paramBackingStore;
};

/// \class N0183IDStore
/// \brief Specialisation of NVMFile for NMEA0183 allowed ID storage

class N0183IDStore {
public:
    N0183IDStore(void);

    void AddID(String const& msgid, bool restart = false);
    void ListIDs(Stream& s);
    void SerialiseIDs(Serialiser *s);

private:
    String  m_backingStore;
};

}

#endif
