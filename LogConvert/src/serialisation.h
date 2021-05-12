/*!\file serialisation.h
 * \brief Simple serialisation of basic types for file storage
 *
 * As part of the logging process, we have to put together binary packets
 * of the input data for storage.  In order to provide some central capability
 * for this, the Serialisable type is used to encapsulate a packet of data to
 * be written, and the Serialiser class can be used to provide the overall
 * encapsulation of data type, length byte, checksum, etc. as required.
 *
 * Copyright (c) 2019, University of New Hampshire, Center for Coastal and Ocean Mapping.
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

#ifndef __SERIALISATION_H__
#define __SERIALISATION_H__

#include <stdint.h>

const int SerialiserVersionMajor = 1; ///< Major version number for the serialiser
const int SerialiserVersionMinor = 0; ///< Minor version number for the serialiser

/// \class Serialisable
/// \brief Provide encapsulation for data to be written to store
///
/// This object provides an expandable buffer that can be used to hold data in preparation
/// for writing to SD card.  The buffer can be pre-allocated to a nominal size before starting
/// in order to avoid having to re-allocate the buffer during preparation (which is slow).

class Serialisable {
public:
    /// \brief Default constructor for the buffer object
    Serialisable(uint32_t size_hint = 255);
    /// \brief Default destructor
    ~Serialisable(void);
    
    /// \brief Add a single byte to the output buffer
    void operator+=(uint8_t b);
    /// \brief Add a 2-byte half-word to the output buffer
    void operator+=(uint16_t w);
    /// \brief Add a 4-byte word to the output buffer
    void operator+=(uint32_t dw);
    /// \brief Add an 8-byte double word to the output buffer
    void operator+=(uint64_t ul);
    /// \brief Add a single-precision floating point number to the ouptut buffer
    void operator+=(float f);
    /// \brief Add a double-precision floating point number of the output buffer
    void operator+=(double d);
    /// \brief Add an array of characters (C-style string) to the output buffer
    void operator+=(const char *p);
    
    uint32_t BufferLength(void) const { return m_nData; }
    uint8_t const *Buffer(void) const { return m_buffer; }
    
private:
    friend class Serialiser;
    uint8_t     *m_buffer;      ///< Pointer to the buffer being assembled
    uint32_t    m_bufferLength; ///< Total size of the buffer currently allocated
    uint32_t    m_nData;        ///< Number of bytes currently used in the buffer
    
    /// \brief Ensure that there is sufficient space in the buffer to add an object of the given size
    void EnsureSpace(size_t s);
};

class Version {
public:
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    Version(uint16_t maj, uint16_t min, uint16_t pat)
    : major(maj), minor(min), patch(pat) {}
};

enum PayloadID {
    Pkt_Version = 0,
    Pkt_SystemTime = 1,
    Pkt_Attitude = 2,
    Pkt_Depth = 3,
    Pkt_COG = 4,
    Pkt_GNSS = 5,
    Pkt_Environment = 6,
    Pkt_Temperature = 7,
    Pkt_Humidity = 8,
    Pkt_Pressure = 9,
    Pkt_NMEAString = 10,
    Pkt_LocalIMU = 11,
    Pkt_Metadata = 12
};

/// \class Serialiser
/// \brief Carry out the serialisation of an object
///
/// This object provides a simple abstract interface for writing a \a Serialisable object
/// into an output stream, the specifics of which are provided by the specialised sub-class.
/// The \a payload_id and a size word are automatically added to the output packets.  A
/// packet with the serialiser version information is written to each file when it is opened
/// so that readers can check on the expected format of the remainder of the file.

class Serialiser {
public:
    /// \brief Default constructor
    Serialiser(Version& n2k, Version& n1k, std::string const& logger_name, std::string const& logger_id);
    
    /// \brief Default destructor
    virtual ~Serialiser(void);
    
    /// \brief Write the payload to file, with header block
    bool Process(PayloadID payload_id, std::shared_ptr<Serialisable> payload);

private:
    std::shared_ptr<Serialisable>   m_version;     ///< Version information to be written
    std::shared_ptr<Serialisable>   m_metadata;    ///< Metadata information to be written
    
    /// \brief Payload serialiser without user-level validity checks
    virtual bool rawProcess(PayloadID payload_id, std::shared_ptr<Serialisable> payload) = 0;
};

/// \class StdSerialiser
/// \brief Serialisation to a standard C FILE pointer

class StdSerialiser : public Serialiser {
public:
    StdSerialiser(FILE *f, Version& n2k, Version& n1k, std::string const& logger_name, std::string const& logger_id)
    : Serialiser(n2k, n1k, logger_name, logger_id), m_file(f) {}
    
private:
    FILE *m_file;
    bool rawProcess(PayloadID payload_id, std::shared_ptr<Serialisable> payload);
};

#endif
